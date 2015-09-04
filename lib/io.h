#ifndef _IO_H_
#define _IO_H_

int read_exact_bytes(void *buffer, int size);
int read_bytes(void *buffer, int size);
int write_bytes(const void *buffer, int size);
int write_string(const char *string);

int get_int();
long get_long();


#endif
