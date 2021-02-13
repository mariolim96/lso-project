#ifndef CLIENT_CLIENT_H
#define CLIENT_CLIENT_H

/********************************************************************/

#define IP_ADDRESS_MAX_LENGHT 16
#define HOSTNAME_MAX_LENGHT 257
#define REQUEST_MAX_LENGHT 2
#define CONFIRM_MAX_LENGHT 3
#define TYPE_COMUNICATION_MAX_LENGHT 2
#define DATE_MAX_LENGHT 20
#define INT_TO_ASCI_MAX_LENGHT 7

#define UPTIME_MAX_LENGHT 13
#define FREERAM_MAX_LENGHT 12
#define PROCS_MAX_LENGHT 7

/********************************************************************/

short int read_number_hosts();
short int write_hosts_list( short int number_hosts);
short int read_request(short int number_host);
void read_index_host(short int number_host);
short int read_host_information(short int number_host);

#endif //CLIENT_CLIENT_H
