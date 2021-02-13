
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "utility.h"

/**
 * @brief Data una stringa verifica se ogni suo carattere è un numero o meno
 * @param str Strinag di caratteri da analizzare
 * @return  true se la stringa in input src è formata solo da caratteri numerici, false in caso contrario
 */

bool validate_number( char *str) {

    while (*str) {
        if(!isdigit(*str)){ //if the character is not a number, return
            return false;
        }
        str++; //point to next character
    }

    return true;
}

/**
 * @brief Data una stringa contenete un indirizzo ip in input, controlla se questo rispetta la forma canonica
 * X.X.X.X dove X è un numero che varia da 0 a 255
 *
 * @bug La fuinzione  altera la stringa passata in input a causa dell'uso della funzione strtok
 * @param ip Stringa contenente l'indirizzo ip da analizzare
 * @return  true se la stringa ip rispetta la forma specificata prima, false in caso contrario
 */

bool validate_ip( char *ip) {

    int i, num, dots = 0;
    char *ptr;

    if (ip == NULL)
        return false;

    ptr = strtok(ip, ".");

    if (ptr == NULL)
        return false;

    while (ptr) {
        if (!validate_number(ptr))
            return false;

        num=atoi(ptr);

        if (num >= 0 && num <= 255) {
            ptr = strtok(NULL, ".");
            if (ptr != NULL)
                dots++; //increase the dot count
        } else
            return false;
    }

    return (dots != 3) ? false : true;
}

/**
 * @brief Parserizza una stringa presa in input src, producendo in output due stringe che contengono l'indirizzo ip e l'hostname
 * del host connesso al server
 *
 * @param src Stringa ricevuta in input
 * @param ip_address  stringa contenente l'indirizzo ip in output
 * @param hostname stringa contenente l'hostname in output
 */

void formact_data_ip_hostname(const char *src, char *ip_address, char *hostname){

    if(src !=NULL){
        int i=0;
        int ip_address_index,hostname_index;
        ip_address_index=hostname_index=0;
        do{
            if(src[i] != '\0'){
                ip_address[ip_address_index]=src[i];
            }
            i++;
            ip_address_index++;

        }while (src[i-1] != '\n' && src[i-1] != '\0');

        ip_address[ip_address_index-1]='\0';

        do{
            if(src[i] != '\0'){
                hostname[hostname_index]=src[i];
            }
            i++;
            hostname_index++;
        }while (src[i-1] != '\n' && src[i-1] != '\0');

        hostname[hostname_index-1]='\0';
    }
}

/**
 * @brief Parserizza una stringa presa in input src, producendo in output tre stringe che contengono
 * le informazioni sullo stato di un host connesso al server
 *
 * @param src Stringa ricevuta in input
 * @param uptime  stringa contenente i secondi trascorsi dall'ultimo boot dell'host connesso al server in output
 * @param freeram stringa contenente la quantità di ram disponibile sull'host connesso al server in output
 * @param procs stringa contenente il numero di processi attualmente in esecuzione sull'host connesso al server in output
 */

void formact_data_info(const char *src, char *uptime, char *freeram, char* procs){

    if(src !=NULL){
        int i=0;
        int uptime_index,procs_index,freeram_index;

        uptime_index=procs_index=freeram_index=0;

        do{
            if(src[i] != '\0'){
                uptime[uptime_index]=src[i];
            }
            i++;
            uptime_index++;

        }while (src[i-1] != '\n' && src[i-1] != '\0');

        uptime[uptime_index-1]='\0';

        do{
            if(src[i] != '\0'){
                procs[procs_index]=src[i];
            }
            i++;
            procs_index++;
        }while (src[i-1] != '\n' && src[i-1] != '\0');

        procs[procs_index-1]='\0';

        do{
            if(src[i] != '\0'){
                freeram[freeram_index]=src[i];
            }
            i++;
            freeram_index++;
        }while (src[i-1] != '\n' && src[i-1] != '\0');

        freeram[freeram_index-1]='\0';
    }
}