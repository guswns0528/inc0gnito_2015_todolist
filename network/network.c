#include "network.h"
#include "../permission/permission.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>

#define SOCKET_QUEUE_SIZE 200

int create_socket(int port)
{
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        return -1;

    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1)
        return -1;

    struct sockaddr_in server_addr;
    server_addr.sin_family = PF_INET;
    server_addr.sin_addr.s_addr = inet_addr("0.0.0.0");
    server_addr.sin_port = htons(port);
    bzero(&server_addr.sin_zero, sizeof(server_addr.sin_zero));

    if (bind(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
        return -1;

    if (listen(sock, SOCKET_QUEUE_SIZE) == -1)
        return -1;

    return sock;
}

void service_forever(int server_socket, service_function_t func)
{
    while (1)
    {
        int client_socket = accept(server_socket, NULL, NULL);
        pid_t child = fork();
        if (child)
        {
            close(client_socket);
        }
        else
        {
            bool is_fd_ready = true;
            is_fd_ready &= dup2(client_socket, STDIN_FILENO) != -1;
            is_fd_ready &= dup2(client_socket, STDOUT_FILENO) != -1;
            is_fd_ready &= dup2(client_socket, STDERR_FILENO) != -1;

            if (is_fd_ready && sandbox())
            {
                close(client_socket);
                const char* greet = "Good to see you\n";

                write(STDOUT_FILENO, greet, strlen(greet) + 1);

                if (func != NULL)
                {
                    func();
                }
            }
            else
            {
                const char* error = "Sorry, temporary unavailable\n";
                write(client_socket, error, strlen(error) + 1);
            }

            _exit(0);
        }
    }
}
