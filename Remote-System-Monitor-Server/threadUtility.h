
#ifndef SERVER_THREADUTILITY_H
#define SERVER_THREADUTILITY_H

#include <stdbool.h>
#include <sys/types.h>

/*****************************************************/

typedef struct agent_thread_utility_t{
    int socket_descriptor;
    unsigned short int next_index;
    bool flag;
    pthread_mutex_t mutex;
}agent_thread_utility_t;

/*****************************************************/

void *agent_controller_thread(void *arg);
void *agent_thread(void *arg);
void *client_thread(void *arg);
void *client_controller_thread(void *arg);
void *periodic_update_thread(void *arg);

void all_resource_thread_relase();
#endif //SERVER_THREADUTILITY_H
