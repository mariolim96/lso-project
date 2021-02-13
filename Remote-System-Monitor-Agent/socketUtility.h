

#ifndef AGENT_SOCKETUTILITY_H
#define AGENT_SOCKETUTILITY_H

/*******************************************/

int socket_descriptor_agent;

/*******************************************/

void init_socket(char * ip_address, unsigned short int port);
bool check_socket_validity();
void run_socket_agent();

#endif //AGENT_SOCKETUTILITY_H
