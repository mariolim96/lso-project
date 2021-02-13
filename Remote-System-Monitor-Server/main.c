#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <string.h>
#include "server.h"
#include "threadUtility.h"

/*******************************************************/

void signal_handler();

/*******************************************************/

int main(int argc, char *argv[]) {

    char* ris=NULL;
    unsigned short port_agent;
    unsigned short port_client;

    // Gestisco i segnali SIGINT SIGTERM E SIGQUIT, utili per la terminazione del server
    signal(SIGINT,signal_handler);
    signal(SIGQUIT,signal_handler);
    signal(SIGTERM,signal_handler);

    // Controllo che il programma sia chiamato con due argomenti come richiesto per il funzionamento
    if(argc != 3) {
        write(STDERR_FILENO,"\nQuando si avvia il server bisogna specificare due porte da utilizzate per l'ascolto di client e agent", 102);
        exit(1);
    }

    // Converto il primo argomento nel rispettivo valore intero e lo asseggno alla porta utilizzata dal server per comunicare con gli agent
    port_agent=strtouq(argv[1], &ris,10);

    // L'argomento passato come porta agent non è della forma corretta
    if (ris == NULL || *ris!= '\0'){
        write(STDERR_FILENO,"\nLa porta agente deve essere un numero compreso tra 1024 e 49151",64);
        exit(1);
    }

    // Converto il secondo argomento nel rispettivo valore intero e lo asseggno alla porta utilizzata dal server per comunicare con i client
    port_client=strtouq(argv[2], &ris,10);

    // L'argomento passato come porta client non è della forma corretta
    if (ris == NULL || *ris!= '\0'){
        write(STDERR_FILENO,"\nLa porta client deve essere un numero compreso tra 1024 e 49151",64);
        exit(1);
    }

    // Controllo che la porta client e la porta agent siano diverse per permettere il corretto funzionamento del del server
    if(port_agent == port_client){
        write(STDERR_FILENO,"\nUtilizzare porte diverse per comunicare con client e agent",59);
        exit(1);
    }

    // Controllo che la porta client e la porta agent inserite non interferiscano con porte di utilizzo comune per il sistema operativo
    if( (port_agent < 1024 || port_agent>49151) || (port_client<1024 || port_client>49151) ){
            write(STDERR_FILENO,"\nUtilizzare porte nel range 1024 - 49152, per non interferire con porte di uso comune del sistema operativo",107);
            exit(1);
    }

    // Inzializzo il server e tutte le sue componenti
    init_server(port_agent,port_client);

    // Avvio i thread che si occuperanno di gestire le connessioni e le rispettive comunicazioni con agent e client e il thread che si occupa di controllare periodicamente la validità dei dati gestiti
    run_client_controller_thread();
    run_agent_controller_thread();
    run_periodic_update_thread();

    // Main loop per non far temrinare il main thread fin quando altri thread legati a questo siano in esecuzione
    while (true){

    }

    return 0;
}

/*******************************************************/

/**
 * @brief Gestisc i segnali SIGTERM, SIGQUIT e SIGINT, in particolare quando questi segnali sono lanciati dal sistema
 * opertivo  vengon eseguiti una sequenza di passi prima di terminare l'esecuzione dell'applicativio.
 * Viene stampato un messaggio che indica la terminazione del server sia su file_log che su file_error_log, vengono rilasciate
 * tutte le risorse e le strutture allocate dinamicamente durante l'esecuzione del server
 *
 *
 */

void signal_handler(){

    time_t ora;
    char pid[PID_MAX_LENGHT];
    char date[DATE_MAX_LENGHT];

    time(&ora);
    formatDateTime(date,ora);
    pid_t pid_val=getpid();
    snprintf(pid,PID_MAX_LENGHT,"%d",pid_val);

    // Stampo su file_log un messaggio che indica la teminazione del processo server e l'ora attuale
    write(file_log_descriptor,"\nEsecuzione del server terminata\tOra attuale: ",46);
    write(file_log_descriptor,date,strlen(date));
    write(file_log_descriptor,"\tProcess id: ",13);
    write(file_log_descriptor,pid,strlen(pid));

    // Stampo su file_error_log un messaggio che indica la teminazione del processo server e l'ora attuale
    write(file_error_log_descriptor,"\nEsecuzione del server terminata\tOra attuale: ",46);
    write(file_error_log_descriptor,date,strlen(date));
    write(file_error_log_descriptor,"\tProcess id: ",13);
    write(file_error_log_descriptor,pid,strlen(pid));

    // Rilascio le risorse utilizzate da thread a dal server allocate dinamicamente durante l'esecuzione
    all_resource_thread_relase();
    relase_resourse_server();
    exit(0);
}

/*******************************************************/
