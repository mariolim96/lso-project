#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "client.h"
#include "socketUtility.h"
#include "utility.h"

/**
 * @brief Legge da socket il numero di host attualmente connessi al server
 *
 * @return Numero di host connessi al server, SOCKET_CLOSED  in caso di errore
 */

short int read_number_hosts(){

    char number_host[INT_TO_ASCI_MAX_LENGHT];
    char *ris=NULL;
    char confirm[CONFIRM_MAX_LENGHT];
    ssize_t read_byte;

    // Inizializzo con tutti 0 le stringhe number_host e confirm
    memset(number_host,'\0',INT_TO_ASCI_MAX_LENGHT);
    memset(confirm,'\0',CONFIRM_MAX_LENGHT);

    // Leggo da socket il numero di host collegati al server
    if( (read_byte=read(socket_descriptor_client,number_host,INT_TO_ASCI_MAX_LENGHT)) <= 0 ){  // Caso in cui la socket lato server è chiusa
        if(read_byte==0){
            return SOCKET_CLOSED;
        }else{
            perror("Errore lettura socket client: ");
            exit(1);
        }

    }
    number_host[read_byte]='\0';
    strcpy(confirm,"ok");

    // Comunica al server l'avvenuta ricezione dei dati
    if(write(socket_descriptor_client,confirm,strlen(confirm)) == -1){    // Caso in cui la socket lato server è chiusa
        perror("Errore scrittura socket client: ");
        exit(1);
    }

    // Ritorno il numero di host collegati al server trasformandolo in una variabile numerica
    return (short)strtol(number_host,&ris,10);
}

/**
 * @brief Stampa a schermo la lista indicizzata di tutti gli host connessi al server
 *
 * @param number_hosts numero di host attualemente connessi al server
 * @return 0 se non viene riscontrato nessun problema, SOCKET_CLOSED in caso di errore
 */

short int write_hosts_list(const short int number_hosts){

    char buffer[HOSTNAME_MAX_LENGHT + IP_ADDRESS_MAX_LENGHT ];
    char ip_address[IP_ADDRESS_MAX_LENGHT];
    char hostname[HOSTNAME_MAX_LENGHT];
    char index[INT_TO_ASCI_MAX_LENGHT];
    ssize_t read_byte;

    // Inizializzo con tutti 0 le stringhe buffer, ip_address e hostname
    memset(buffer,'\0',HOSTNAME_MAX_LENGHT + IP_ADDRESS_MAX_LENGHT);
    memset(ip_address,'\0',IP_ADDRESS_MAX_LENGHT);
    memset(hostname,'\0',HOSTNAME_MAX_LENGHT);

    // Ciclo sul numero di host collegati al server
    for(short int i=0; i<number_hosts;i++ ) {
        // Per ogni host collegato al server leggo da socket l'hostname e il suo indirizzo ip
        if ((read_byte = read(socket_descriptor_client, buffer, HOSTNAME_MAX_LENGHT + IP_ADDRESS_MAX_LENGHT)) <= 0) { // Caso in cui la socket lato server è chiusa
            if (read_byte == 0) {
                return SOCKET_CLOSED;
            } else {
                perror("Errore lettura socket client: ");
                exit(1);
            }
        }

        buffer[read_byte] = '\0';
        formact_data_ip_hostname(buffer, ip_address, hostname);  // parserizza le informazioni su un host letti da socket
        // Inizialiazzo con tutti 0 la stringa index
        memset(index, '\0',INT_TO_ASCI_MAX_LENGHT);
        snprintf(index,INT_TO_ASCI_MAX_LENGHT,"%hu",i);
        // Mostro su standard output le informazioni appena comunicate dal server
        write(STDOUT_FILENO,"\n",1);
        write(STDOUT_FILENO,index,strlen(index));
        write(STDOUT_FILENO, ")",1);
        write(STDOUT_FILENO, "\tINDIRIZZO IP:  ", 16);
        write(STDOUT_FILENO, ip_address, strlen(ip_address));
        write(STDOUT_FILENO, "\tHOSTNAME:  ", 12);
        write(STDOUT_FILENO, hostname, strlen(hostname));
    }

    return 0;
}

/**
 * @brief Legge da standard input un operazione che si vuole effettuare tra le tre possibili scelte e la invia tramite socket al server
 * Le operazioni possibili sono:
 * REQUEST_HOSTS_LIST -->   ottenere una lista di host connessi al server
 * REQUEST_HOST_INFORMATION  --> ottenere informazioni sullo stato di un host connesso al server
 * REQUEST_EXIT 3 --> terminare il programma
 *
 * @return La richiesta da effettuare oppure SOCKET_CLOSED in caso di errore
 */

short int read_request(short int number_host){

    char *ris=NULL;
    char request[REQUEST_MAX_LENGHT];
    short int request_val;

    do{
        write(STDOUT_FILENO,"\nDigitare:\n1) Aggiornare la lista host connessi\t2) Visualizzare informazioni determinato host\t3) Terminare\n",107);
        // Inizializzo con tutti 0 la stringa request
        memset(request,'\0',REQUEST_MAX_LENGHT);
        // Leggo da standar input un valore numerico, questo indica l'operazione che l'utenete sceglie, deve essere necesseriamente essere un carattere numerico tra 1 e 3
        read(STDIN_FILENO,request,REQUEST_MAX_LENGHT);
        request[1]='\0';
        // Converto in una variabile numerica la scelta effettuata dall'utente
        request_val=(short)strtol(request,&ris,10);
        if(strcmp(ris,"") == 0){
            ris=NULL;
        }
    }while(ris != NULL || request_val < 1  || request_val > 3);

    if(request_val== 2 && number_host == 0){ // Caso in cui il numero di host sia 0 e la richiesta sia di mostrare informazioni di un determinato host connesso alla rete, la richiesta non è valida e non comunico niente al server
        return request_val;
    } else{// Caso in cui il numero di host sia maggiore di 0 e la richiesta sia di mostrare informazioni di un determinato host connesso alla rete
        // Comunico al server tramite socket la richiesta di operazione da effettuare
        if ( write(socket_descriptor_client,request,1) == -1){
            perror("Errore scrittura socket client: ");
            exit(1);
        } else {
            return request_val;
        }
    }

}

/**
 * @brief Leggere da standard input l'indice di un host connesso al server di cui si vuole conoscere le informazioni
 * e lo comunica tramite socket al server
 *
 * @param number_host_val Numero di host connessi al server
 */

void read_index_host(short int number_host_val){

    char index[INT_TO_ASCI_MAX_LENGHT];
    char number_host[INT_TO_ASCI_MAX_LENGHT];
    short int index_val;
    char *ris=NULL;

    do{
        // Inizializzo con tutti 0 la stringa index
        memset(number_host,'\0',INT_TO_ASCI_MAX_LENGHT);
        snprintf(number_host,INT_TO_ASCI_MAX_LENGHT,"%d",(number_host_val-1));
        write(STDOUT_FILENO,"\nINSERIRE LA SCELTA DI QUALE HOST SI VUOLE VISUALIZZARE ( inserire un valore compreso tra 0 e " ,94 );
        write(STDOUT_FILENO,number_host,strlen(number_host));
        write(STDOUT_FILENO," ): ",4);
        // Inizializzo con tutti 0 la stringa index
        memset(index,'\0',INT_TO_ASCI_MAX_LENGHT);
        // Leggo da standar input un valore numerico, questo indica la poszione dell'host nella lista mostrata su standard output, il valore deve essere necessariamente nel range di valori mostrati a video
        read(STDIN_FILENO,index,INT_TO_ASCI_MAX_LENGHT);
        index[strlen(index)-1]='\0';

        // Converto in una variabile numerica la posizione scelta dall'utente
        index_val=(short)strtol(index,&ris,10);
        if(strcmp(ris,"")==0){
            ris=NULL;
        }
    }while(ris != NULL || index_val< 0 || index_val >= number_host_val);

    nanosleep(&timespec,NULL);

    // Comunico al server tramite socket la scelta effettuata
    if( write(socket_descriptor_client,index,strlen(index)) == -1){
        perror("Errore scrittura socket client: ");
        exit(1);
    }
}

/**
 * @brief Legge da socket le informazioni sullo stato di un determinato host connesso al server
 *
 * @param number_host  numero di host connessi al server
 * @return  0 se le operazioni vengono effettuate con successo, SOCKET_CLOSED in caso di errore
 */

short int read_host_information(short int number_host){

    ssize_t read_byte;
    char info[UPTIME_MAX_LENGHT+FREERAM_MAX_LENGHT+PROCS_MAX_LENGHT];
    char uptime[UPTIME_MAX_LENGHT],freeram[FREERAM_MAX_LENGHT],procs[PROCS_MAX_LENGHT];
    char type_comunication[TYPE_COMUNICATION_MAX_LENGHT];
    char date[DATE_MAX_LENGHT];

    // Inizializzo con tutti 0 le stringhe type_comunication, info, date, uptime, freeram e procs
    memset(type_comunication,'\0',TYPE_COMUNICATION_MAX_LENGHT);
    memset(info,'\0',UPTIME_MAX_LENGHT+FREERAM_MAX_LENGHT+PROCS_MAX_LENGHT);
    memset(date,'\0',DATE_MAX_LENGHT);
    memset(uptime,'\0',UPTIME_MAX_LENGHT);
    memset(freeram,'\0',FREERAM_MAX_LENGHT);
    memset(procs,'\0',PROCS_MAX_LENGHT);

    // leggo il numero di host collegati al server
    read_index_host(number_host);

    // Legge  il tipo  di comunicazione che  riceverà dal server, HOST CONNESSO oppure HOST DISCONNESSO
    if( (read_byte=read(socket_descriptor_client,type_comunication ,TYPE_COMUNICATION_MAX_LENGHT)) <= 0){  // Caso in cui la socket lato server è chiusa
        if(read_byte==0){
            return SOCKET_CLOSED;
        }else{
            perror("Errore lettura socket client: ");
            exit(1);
        }
    }

    type_comunication[1]='\0';
    nanosleep(&timespec,NULL);

    // Invio conferma ricezione dei dati al server
    if( write(socket_descriptor_client,"ok",CONFIRM_MAX_LENGHT) == -1){  // Caso in cui la socket lato server è chiusa
        perror("Errore scrittura socket client: ");
        exit(1);
    }


    if(strcmp(type_comunication,"0")==0){       // Caso: HOST DISCONNESSO
        // Leggo da socket le informazioni inviate dal server sulla data dell'ultima comunicazione del server con quel host
        if( (read_byte=read(socket_descriptor_client, date ,DATE_MAX_LENGHT)) <= 0){  // Caso in cui la socket lato server è chiusa
            if(read_byte == 0){
                return SOCKET_CLOSED;
            }else{
                perror("Errore lettura socket client: ");
                exit(1);
            }
        }

        date[strlen(date)]='\0';
        // Mostro su standard output le informazini ricevute dal server
        write(STDOUT_FILENO,"\nHost disconnesso\tUltima comunicazione: ",40);
        write(STDOUT_FILENO, date, strlen(date));
    } else{ // Caso: HOST CONNESSO
        // Leggo da socekt le informazioni sull'host inviate dal server
        if( (read_byte=read(socket_descriptor_client,info ,UPTIME_MAX_LENGHT+FREERAM_MAX_LENGHT+PROCS_MAX_LENGHT)) <= 0){ // Caso in cui la socket lato server è chiusa
            if(read_byte==0){
                return SOCKET_CLOSED;
            }else{
                perror("Errore lettura socket client: ");
                exit(1);
            }
        }

        formact_data_info(info,uptime,freeram,procs);   // parserizza i dari sullo stato di un host ricevuti letti da socket
        // Mostro su standard output le informazini ricevute dal server
        write(STDOUT_FILENO,"\nUptime: ",9);
        write(STDOUT_FILENO,uptime, strlen(uptime));
        write(STDOUT_FILENO,"\nFreeram: ",10);
        write(STDOUT_FILENO,freeram, strlen(freeram));
        write(STDOUT_FILENO,"\nProcs: ",8);
        write(STDOUT_FILENO,procs, strlen(procs));
    }

    return 0;
}