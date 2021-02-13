#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "utility.h"
#include "socketUtility.h"

/*******************************************/

#define IP_ADDRESS_MAX_LENGHT 16

/*******************************************/

void signal_handler();
void signal_handler2();

/*******************************************/

int main(int argc, char *argv[]) {

    char ip_address[IP_ADDRESS_MAX_LENGHT];
    char *ris=NULL;
    unsigned short port;

    // Inizializzo la stringa ip_address con tutti 0
    memset(ip_address,'\0',IP_ADDRESS_MAX_LENGHT);

    // Gestisco i segnali SIGTERM, SIGPIPE, SIGINT e SIGQUIT
    signal(SIGTERM,signal_handler);
    signal(SIGQUIT,signal_handler);
    signal(SIGINT,signal_handler);
    signal(SIGPIPE,signal_handler2);

    // Verifico che il programma sia stao chimato con due parametri ovvero l'indirizzo ip del server e la porta con cui comunicare
    if(argc != 3) {
        write(STDERR_FILENO,"\nBisongna passare come parametri l'inidrizzzo IP del server a cui collegarsi e la porta da utilizzare per la comunicazione",122);
        exit(1);
    }

    // Copio il l'indirizzo ip passtao come argomento al programma nella stringa ip_addrss
    strcpy(ip_address,argv[1]);
    //ip_address[strlen(argv[1])+1]='0';

    // Controllo se l'ip passato come argomento al programma ha una forma valida
    if(!validate_ip(ip_address){
       write(STDERR_FILENO,"\nL'indirizzo IP non è scritto nella forma corretta X.X.X.X (X numero compreso tra 0 e 255)",90);
       exit(1);
    }

    // Converto la porta passata come argomento al programma da un stringa asci ad un tipo numerico
    port=strtoul(argv[2],&ris,10);

    if (ris == NULL || *ris!= '\0' || port <1024 || port > 49151){
        write(STDERR_FILENO,"\nLa porta deve essere un valore numerico compreso tra 1024 e 49151",87);
        exit(1);
    }

    // inzializzo la socket utilizzata per la comunicazione con il server con i parametri passti in input al programma
    init_socket(ip_address, port);
    // Avvio la comunicazione con il server
    run_socket_agent();

    return 0;
}

/**
 * @brief Funzione utile per cattuarare i segnali SIGTERM SIGSTOP SIGINT
 *
 */

void signal_handler(){

    write(STDERR_FILENO,"\nProgramma terminato volutamente dall'utente",44);

    if(socket_descriptor_agent !=0){
        close(socket_descriptor_agent);
    }

    exit(1);

}

/**
 * @brief Funzione utile per cattuarare i segnali SIGPIPE
 */

void signal_handler2(){

    write(STDERR_FILENO,"\nServer non più disponibile",28);
    if(socket_descriptor_agent !=0){
        close(socket_descriptor_agent);
    }
    exit(1);

}