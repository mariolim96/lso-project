

#ifndef CLIENT_SOCKETUTILITY_H
#define CLIENT_SOCKETUTILITY_H

/*******************************************/

#define SOCKET_CLOSED -1

/*******************************************/

int socket_descriptor_client;
struct timespec timespec;

/*******************************************/

void init_socket(char * ip_address, unsigned short int port);
void run_socket();

#endif //CLIENT_SOCKETUTILITY_H
