
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "socketUtility.h"
#include "client.h"
#include "stdbool.h"


/**********************************************/

#define REQUEST_HOSTS_LIST 1
#define REQUEST_HOST_INFORMATION 2
#define REQUEST_EXIT 3

/**********************************************/

/**
 * @brief inizializza una socket per predisporre la comunicazione con il server,
 *
 * @param ip_address inidirizzo IP del server al quale ci si vuole connettere
 * @param port  porta da utilizzare per la comunicazione con il server
 */

void init_socket(char * ip_address, unsigned short int port){

    struct sockaddr_in socket_server_addr;
    // inzializzo la struttura  socket_server_addr con tutti 0
    memset(&socket_server_addr,'\0', sizeof(socket_server_addr));
    // inzializzo la struttura  socket_server_addr con i valori di indirizzo ip e porta passati come argomento al programma
    socket_server_addr.sin_family=AF_INET;
    socket_server_addr.sin_addr.s_addr=inet_addr(ip_address);
    socket_server_addr.sin_port=htons(port);

    // Creo la socket utilizzata per comunicare con il server
    if( (socket_descriptor_client=socket(PF_INET,SOCK_STREAM,0))< 0 ){
        perror("Errore creazione socket client: ");
        exit(1);
    }

    // Connetto la socket utilizzata per comunicare con il server
    if( connect(socket_descriptor_client,(struct sockaddr*)&socket_server_addr, sizeof(socket_server_addr)) != 0){
        perror("Errore connect socket client: ");
        exit(1);
    }

    // Imposto i valori della variabile globale timespec utilizzata per sincronizzare le comuninacazuini tramite socket di client e server
    timespec.tv_sec=0;
    timespec.tv_nsec=10000000;
}

/**
 * @brief avvia  la comunicazione con il server tramite socket, il flusso è il seguente:
 * 1)  Richiede al server una lista di host connessi
 * 2)  In un loop infinito chiede di far scegliere all'utente che operazione tra le tre disponibili eseguire
 * 3)  Esegue l'operazione richiesta e cicla nuovamente
 *
 */

void run_socket(){

    short int number_host;
    short int request_val;

    nanosleep(&timespec,NULL);
    write(STDOUT_FILENO,"\nConnessione avvenuta con successo, Client Attivo",49);
    // Comunico la scelta dell'operazione da effettuare al server tramite socket, in questo caso 1 indica come prima richiesta di comunicare al client la lista di tutti gli host presenti nel server
    if (write(socket_descriptor_client,"1",1) == -1){ // Caso in cui la socket lato server è chiusa
        perror("Errore scrittura socket client: ");
        exit(1);
    }
    // Leggo da socket il numero di host attualmente collegati al server
    if ((number_host=read_number_hosts()) == SOCKET_CLOSED){ // Caso in cui la socket lato server è chiusa
        write(STDERR_FILENO,"\nServer non piu disponibile",27);
        if(socket_descriptor_client!= 0){
            close(socket_descriptor_client);
        }
        exit(1);
    }

    // Mostro su standard output la lista degli host collegati al server
    if(write_hosts_list(number_host)!=0){ // Caso in cui la socket lato server è chiusa
        write(STDERR_FILENO,"\nServer non piu disponibile",27);
        if(socket_descriptor_client!= 0){
            close(socket_descriptor_client);
        }
        exit(1);
    }

    while(true){
        // Leggo la scelta dell'operazione da effettuare dell'utente
        request_val=read_request(number_host);
        switch (request_val){
            case REQUEST_HOSTS_LIST:{  // Richiesta di comunicare la lista di host connessi al server
                nanosleep(&timespec,NULL);

                // Leggo da socket il numero di host attualmente collegati al server
                if ( (number_host=read_number_hosts()) == SOCKET_CLOSED){ // Caso in cui la socket lato server è chiusa
                    write(STDERR_FILENO,"\nServer non piu disponibile",27);
                    if(socket_descriptor_client!= 0){
                        close(socket_descriptor_client);
                    }
                    exit(1);
                }

                // Mostro su standard output la lista degli host collegati al server
                if(write_hosts_list(number_host) != 0){ // Caso in cui la socket lato server è chiusa
                    write(STDERR_FILENO,"\nServer non piu disponibile", 27);
                    if(socket_descriptor_client != 0){
                        close(socket_descriptor_client);
                    }
                    exit(1);
                }
            }
                break;
            case REQUEST_HOST_INFORMATION: { // Richiesta di mostrare informazioni su un host scelto dall'utente
                if(number_host!=0) {
                    // Leggo da socket le informazioni su un host collegato al server scelto dall'utente
                    if (read_host_information(number_host) != 0) { // Caso in cui la socket lato server è chiusa
                        write(STDERR_FILENO, "\nServer non piu disponibile", 27);
                        if (socket_descriptor_client != 0) {
                            close(socket_descriptor_client);
                        }
                        exit(1);
                    }
                }else { // Caso in cui la varibile number host contiene il valore 0, non sono presenti host connessi alla rete, si deve chiedere al server informazioni aggiornate
                    write(STDOUT_FILENO,"\nNon ci sono host collegati alla rete attualmente, provare ad aggiornare la lista degli host",92);
                }
            }
                break;
            case REQUEST_EXIT: { // Richiesta di terminare l'esecuzione del programma
                // Mostro su standard outupt la comunicazione dell'effettiva terminazione del programma
                write(STDOUT_FILENO, "\nProgramma terminato con successo", 33);
                close(socket_descriptor_client);
                exit(0);
            }
            default:{
            }
                break;
        }
    }
}