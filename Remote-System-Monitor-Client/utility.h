

#ifndef AGENT_UTILITY_H
#define AGENT_UTILITY_H

#include <stdbool.h>

bool validate_number( char *str);
bool validate_ip( char *ip);
void formact_data_ip_hostname(const char *src, char *ip_address, char *hostname);
void formact_data_info(const char *src, char *uptime, char *freeram, char* procs);

#endif //AGENT_UTILITY_H
