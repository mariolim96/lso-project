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

    // Inizializzo con tuutti 0 la stringa ip_address
    memset(ip_address,'\0', IP_ADDRESS_MAX_LENGHT);
    // Imposto la gestione dei segnali SIGTERM, SIGQUIT, SIGINT e  SIGPIPE
    signal(SIGTERM,signal_handler);
    signal(SIGQUIT,signal_handler);
    signal(SIGINT,signal_handler);
    signal(SIGPIPE,signal_handler2);

    // Verifico che il nuemero dei paramtri passati al programma siano 2, ovvero l'indirizzo ip del server e la porta con cui comunicare
    if(argc != 3) {
        write(STDERR_FILENO,"\nBisongna passare come parametri l'inidrizzo IP del server a cui collegarsi e la porta da utilizzare per la comunicazione",122);
        exit(1);
    }

    // Copio l'indirizzo ip passato come argomento al programma in una nuova stringa, dato che la funzione validate_ip ne modifica il contenuto
    strcpy(ip_address,argv[1]);
    ip_address[strlen(argv[1])]='\0';

    // Verifico che l'indirizzo ip passato come input al programma sia stato inserito nella forma valida
    if(!validate_ip(argv[1])){
        write(STDERR_FILENO,"\nL'indirizzo ip non è scritto nella forma corretta X.X.X.X (X numero compreso tra 0 e 255)",90);
        exit(1);
    }

    // Converto in una varibile numerica il valore della porta passato come imput al programma
    port=strtoul(argv[2],&ris,10);

    // Verifico che la porta utilizzata sia nel range di porte preveste dallo standard
    if (ris == NULL || *ris!= '\0' || port <1023 || port > 49151){
        write(STDERR_FILENO,"\nLa porta deve essere un valore numerico compreso tra 1024 e 49151",87);
        exit(1);
    }

    // Inizializzare la socket per comunicare con il server
    init_socket(ip_address, port);
    // Avviare comunicazione con il server
    run_socket();

    return 0;
}


/**
 * @brief Funzione utile per cattuarare i segnali SIGTERM SIGSTOP SIGINT
 *
 */

void signal_handler(){

    write(STDERR_FILENO,"\nProgramma terminato volutamente dall'utente",44);
    if(socket_descriptor_client !=0){
        close(socket_descriptor_client);
    }
    exit(1);
}

/**
 * @brief Funzione utile per cattuarare i segnali SIGPIPE
 *
 */

void signal_handler2(){

    write(STDERR_FILENO,"\nServer non più disponibile",27);
    if(socket_descriptor_client !=0){
        close(socket_descriptor_client);
    }
    exit(1);
}