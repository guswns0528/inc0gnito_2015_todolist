#ifndef _NETWORK_H_
#define _NETWORK_H_

#include "network-defs.h"

int create_socket(int port);
void service_forever(int server_socket, service_function_t func);

#endif
