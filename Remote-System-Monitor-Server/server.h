

#ifndef SERVER_SERVER_H
#define SERVER_SERVER_H

#include <bits/types/time_t.h>
#include <sys/types.h>
#include <stdbool.h>

/*****************************************************/

#define MAX_QUEUE_LENGHT 200
#define NUMBER_HOST_CONNECTED_INIT 20
#define MAX_ERROR_LOG_BUFFER 200
#define DATE_MAX_LENGHT 20
#define PID_MAX_LENGHT 12
#define TID_MAX_LENGHT 12

/*****************************************************/

typedef struct host_t{
    char *ip_address;
    char *hostname;
    unsigned long free_ram;
    unsigned short procs;
    long uptime;
    time_t last_communication;
    pthread_mutex_t mutex;
    bool isActive;
}host_t;

typedef struct hosts {
    host_t ** hosts;
    unsigned short dim;
    unsigned short next_index;
    pthread_mutex_t mutex;
}hosts;

/*****************************************************/

hosts connected_hosts;
int socket_agent_descriptor_controller;
int socket_client_descriptor_controller;
pthread_t pthread_agent_controller;
pthread_t pthread_client_controller;
pthread_t pthread_periodic_update;

int file_log_descriptor;
int file_error_log_descriptor;

char* error_log_buffer;
pthread_mutex_t error_log_buffer_mutex;
pthread_mutex_t log_file_mutex;

/*****************************************************/

void init_server( unsigned short int port_agent, unsigned short int port_client);
void run_client_controller_thread();
void run_agent_controller_thread();
void run_periodic_update_thread();

void formatDateTime( char  dateTimeStr[DATE_MAX_LENGHT], time_t mytime);
void print_on_log(const char msg[200]);
void print_on_error_log(const char msg[200]);
void relase_resourse_server();

#endif //SERVER_SERVER_H
