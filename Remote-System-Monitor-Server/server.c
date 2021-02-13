

#include "server.h"
#include "threadUtility.h"
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>


/**
 * @brief Questa funzione inizializza il server,  impostando le socket in ascolto per client e agent,
 * inizializza la struttura conneceted_host che tiene traccia di tutti gli host connessi al server, crea una maschera
 * personalizzata dei segnali indicando al sistema operativo di ignorare il segnale SIGPIPE, e apre due file
 * utilizzati come registri di log dal server.
 *
 * @param port_agent Porta utilizzata dalle socket per comunicare con gli agent
 * @param port_client Porta utilizzata dalle socket per comunicare con i client
 *
 */

void init_server(const unsigned short int port_agent,const unsigned short int port_client){
    // porte per i client e server 
    struct sockaddr_in socket_client_addr,socket_agent_addr;
    int flag=1;
    time_t ora;
    char pid[PID_MAX_LENGHT];
    char date[DATE_MAX_LENGHT];
    sigset_t newmask,oldmask;
    struct stat st = {0};

    // Inizializzo  a 0 ogni singolo byte delle strutture socket_client_addr,socket_agent_addr
    memset(&socket_client_addr,'\0', sizeof(socket_client_addr));
    memset(&socket_agent_addr,'\0', sizeof(socket_agent_addr));

    // Imposto per le strutture socket_client_addr,socket_agent_addr i valori corretti per inziare una comunicazione tramite socket TCP con client e agent sulle porte scelte
    socket_client_addr.sin_family = socket_agent_addr.sin_family = AF_INET;
    //htons e htonl sono utilizzati per il reverse dei byte(perche devono essere in network byte order)
    socket_client_addr.sin_addr.s_addr = socket_agent_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    socket_agent_addr.sin_port = htons(port_agent);
    socket_client_addr.sin_port = htons(port_client);
    //viene creato la cartella  di log nel caso in cui non esistesse 
    if (stat("Log", &st) == -1) {
        if(errno == ENOENT){
            if (mkdir("Log", 0700) !=0){
                perror("\nErrore creazione cartella Log: ");
                exit(1);
            }
        }else {
            perror("\nErrore stat: ");
            exit(1);
        }
    }

    // Apro in modalità append, tutti i permessi per l'utente un file di error_log
    if( (file_error_log_descriptor = open("Log/Server_error_log.txt", O_CREAT|O_APPEND| O_WRONLY,S_IRWXU)) < 0){
        perror("Errore apertura file log server: ");
        exit(1);
    }

    // Apro in modalità append, tutti i permessi per l'utente un file di log
    if( (file_log_descriptor = open("Log/Server_log.txt",O_CREAT|O_APPEND| O_WRONLY,S_IRWXU)) <  0){
        perror("\nErrore apertura file log server: ");
        exit(1);
    }

    if (time(&ora) == ((time_t) -1)){
        print_on_error_log("\nErrore chimata time: ");
        exit(1);
    }
    //Inizializzo date alla data attuale
    formatDateTime(date, ora);
    
    pid_t pid_val = getpid();
    snprintf(pid,PID_MAX_LENGHT,"%d",pid_val);

    // Stampo su file_error_log un messaggio di inizializzazione
    write(file_error_log_descriptor,"\n\nAvvio nuova sessione server\tOra attuale: ",43);
    write(file_error_log_descriptor,date,strlen(date));
    write(file_error_log_descriptor,"\tProcess id: ",13);
    write(file_error_log_descriptor,pid,strlen(pid));

    // Stampa su file_log un messaggio di inizializzazione
    write(file_log_descriptor,"\n\nAvvio nuova sessione server\tOra attuale: ",43);
    write(file_log_descriptor,date,strlen(date));
    write(file_log_descriptor,"\tProcess id: ",13);
    write(file_log_descriptor,pid,strlen(pid));

    // Alloco la variabile globale error_log_buffer e mi assicuro che ci sia spazio a sufficienza nel sistema per questa allocazione
    error_log_buffer=(char*)calloc(MAX_ERROR_LOG_BUFFER,sizeof(char*));
    assert(error_log_buffer != 0);

    // Inizializzo i mutex che protegono la scrittura sul buffer error_log_buffer, il quale è una variabile globale, e sul file di log,
    // in questo modo rendo thread safe ogni scrittura su i file di log assicurando che quando si vuole scrivere un messaggio su un file di log si
    // deve ottenere prima il conrollo di un mutex, e solo quando se ne in possesso si può effetture una scrittura.
    if(pthread_mutex_init(&error_log_buffer_mutex,NULL)!=0){
        print_on_error_log("\nErrore inizializzazione mutex error_log_file_mutex: ");
        exit(1);
    }
    if(pthread_mutex_init(&log_file_mutex,NULL) != 0){
        print_on_error_log("\nErrore inizializzazione mutex log_file_mutex: ");
        exit(1);
    }

    // Creo la socket TCP usata per comunicare con gli agent
    if((socket_agent_descriptor_controller=socket(PF_INET,SOCK_STREAM,0)) <  0){
        print_on_error_log("\nErrore creazione socket agent: ");
        exit(1);
    }
    // Imposto per la socket usata per comunicare con gli agent l'opzione di riusare la porta scelta se già in uso da qualche altro processo
    if(setsockopt(socket_agent_descriptor_controller, SOL_SOCKET,SO_REUSEPORT, &flag, sizeof(int)) != 0){
        print_on_error_log("\nErrore set impostazione socket agent: ");
        exit(1);
    }
    // Effettua il bind della socket TCP usata per comunicare con gli agent
    if( bind(socket_agent_descriptor_controller, (struct sockaddr*)&socket_agent_addr, sizeof(socket_agent_addr)) != 0 ){
        print_on_error_log("\nErrore bind socket agent: ");
        exit(1);
    }
    // Metto in ascolto la socket usata per comunicare con gli agent, imposto la dimensione della coda di attessa al valore 200
    if(listen(socket_agent_descriptor_controller, MAX_QUEUE_LENGHT) != 0){
        print_on_error_log("\nErrore listen socket agent: ");
        exit(1);
    }


    // Creo la socket TCP usata per comunicare con i client
    if((socket_client_descriptor_controller=socket(PF_INET,SOCK_STREAM,0)) <  0){
        perror("Errore creazione socket client!");
        exit(1);
    }

    // Imposto per la socket usata per comunicare con i client l'opzione di riusare la porta scelta se già in uso da qualche altro processo
    if( setsockopt(socket_client_descriptor_controller,SOL_SOCKET,SO_REUSEPORT,&flag, sizeof(int)) != 0){
        perror("Errore set impostazione  socket client!");
        exit(1);
    }

    // Effettua il bind della socket TCP usata per comunicare con i client
    if( bind(socket_client_descriptor_controller,( struct sockaddr*)&socket_client_addr, sizeof(socket_client_addr)) != 0 ){
        perror("Errore bind socket client!");
        exit(1);
    }

    // Metto in ascolto la socket usata per comunicare con i client, imposto la dimensione della coda di attessa al valore 200
    if(listen(socket_client_descriptor_controller,MAX_QUEUE_LENGHT) != 0){
        perror("Errore listen socket client!");
        exit(1);
    }
    //************************************************************
    // Inizializzo la variabile gloabale connected_hosts, questa struttura ci permette di tenere traccia di tutti gli host attualmente connessi al server e le loro relative informazioni
    connected_hosts.hosts=(host_t**)calloc( NUMBER_HOST_CONNECTED_INIT , sizeof(host_t));
    assert(connected_hosts.hosts != 0);
    for (int i = 0; i <NUMBER_HOST_CONNECTED_INIT ; ++i) {
        connected_hosts.hosts[i]=NULL;
    }
    connected_hosts.dim=NUMBER_HOST_CONNECTED_INIT;
    connected_hosts.next_index=0;
    if(pthread_mutex_init(&connected_hosts.mutex,NULL) !=0){
        print_on_error_log("\nErrore inizializzazione mutex connected_host: ");
        exit(1);
    }
    
    // Creo un insieme di segnali, in questo caso composto dal solo segnale SIGPIPE, e poi setto un nuova maschera dei segnali per far si che il segnale SIGPIPE sia bloccato e non faccia terminare il server
    // questo ci serve dato che ogni volta che tentiamo di scivere su una socket che potrebbe essere chiusa lato client o lato agent, questa non deve far bloccare l'esecuzione del server, ma semplicemente
    // far terminare il thread che stava gestendo la comunicazione con quel client o agent
    sigemptyset(&newmask);
    sigaddset(&newmask, SIGPIPE);
    if (sigprocmask(SIG_BLOCK, &newmask, &oldmask) < 0) {
        print_on_error_log("\nErrore settaggio maschera: ");
        exit(1);
    }

}

/**
 *  @brief crea un nuovo thread che si occupa di gestire le connessioni alla socket
 *  utilizzata dagli agent per comunicare con il server,  in questo modo dato che sarà
 *  un nuovo thread a gestire la socket, non blocco il flusso d'esecuzione del main thread
 *  che in questo modo può prosegeguire con altri compiti
 *  Il nuovo thread creato viene impostato a detached, dato che questo thread non ha necessità
 *  di comunicare nessun dato con il server, e la sua terminazione è correlata soltanto alla
 *  chiusura del server
 *
 */

void run_agent_controller_thread(){

    // Creo il thread che si occupa di gestire le connessioni con gli agent
    if( pthread_create(&pthread_agent_controller, NULL, agent_controller_thread,NULL)!=0){
        print_on_error_log("\nErrore creazione agent controller thread: ");
        exit(1);
    }
    print_on_log("\nCreato il thread controller agent");

    // Imposto il thread che si occupa di gestire le connessioni con gli agent di tipo detached
    if(pthread_detach(pthread_agent_controller)!= 0){
        print_on_error_log("\nErrore set detach state thread agent controller: ");
        exit(1);
    }


}

/**
 *  @brief crea un nuovo thread che si occupa di gestire le connessioni alla socket
 *  utilizzata dai client per comunicare con il server,  in questo modo dato che sarà
 *  un nuovo thread a gestire la socket, non blocco il flusso d'esecuzione del main thread
 *  che in questo modo può prosegeguire con altri compiti
 *  Il nuovo thread creato viene impostato a detached, dato che questo thread non ha necessità
 *  di comunicare nessun dato con il server, e la sua terminazione è correlata soltanto alla
 *  chiusura del server
 *
 */
void run_client_controller_thread(){

    // Creo il thread che si occupa di gestire le connessioni con i client
    if( pthread_create(&pthread_client_controller, NULL, client_controller_thread,NULL)!=0){
        print_on_error_log("\nErrore creazione client controller thread: ");
        exit(1);
    }
    print_on_log("\nCreato il thread controller client");

    // Imposto il thread che si occupa di gestire le connessioni con i client di tipo detached
    if(pthread_detach(pthread_client_controller)!= 0){
        print_on_error_log("\nErrore set detach state thread client controller: ");
        exit(1);
    }

}

/**
 *  @brief crea un nuovo thread che si occupa di effetturare controlli periodicamente sulla
 *  struttura connected_hosts, verificando lo stato di attività di ogni singolo host connesso al server.
 *  Il nuovo thread creato viene impostato a detached, dato che questo thread non ha necessità
 *  di comunicare nessun dato con il server, e la sua terminazione è correlata soltanto alla
 *  chiusura del server
 */

void run_periodic_update_thread(){

    // Creo il thread che si occupa di aggiornare la struttura dati connected_hosts verificandone la validità
    if( pthread_create(&pthread_periodic_update, NULL,periodic_update_thread,NULL)!=0){
        print_on_error_log("\nErrore creazione periodic_update thread: ");
        exit(1);
    }
    print_on_log("\nCreato il thread periodic_update");

    // Imposto il thread che si occupa di aggiornare la struttura dati connected_hosts verificandone la validità di tipo detached
    if(pthread_detach(pthread_periodic_update)!= 0){
        print_on_error_log("\nErrore set detach state thread periodic_update: ");
        exit(1);
    }

}

/**
 *  @brief Permette di rilasciare ogni risorsa globale allocata dinamicamente durante l'esecuzione del server
 *
 */

void relase_resourse_server(){

    // Termino i thread pthread_periodic_update,pthread_client_controller, pthread_agent_controller che fanno uso delle risorse
    // condivise del server
    pthread_kill(pthread_periodic_update, 9);
    pthread_kill(pthread_client_controller, 9);
    pthread_kill(pthread_agent_controller, 9);

    // Rilascio ogni spazio in memoria allocato dinamicamente durante l'uso della struttura connected_hosts,
    // rilascio lo spazio allocato per la struttura connected_hosts, distruggo ogni mutex allocato dinamicamente in questa struttura,
    // ditruggo i mutex allocati dinamicamente utilizzati per i file di log
    for(int i=0;i<connected_hosts.dim;i++){
        if(connected_hosts.hosts[i]!=NULL){
            pthread_mutex_destroy(&connected_hosts.hosts[i]->mutex);
            free(connected_hosts.hosts[i]->hostname);
            free(connected_hosts.hosts[i]->ip_address);
            free(connected_hosts.hosts[i]);
        }
    }
    free(connected_hosts.hosts);
    pthread_mutex_destroy(&connected_hosts.mutex);
    free(error_log_buffer);
    pthread_mutex_destroy(&error_log_buffer_mutex);
    pthread_mutex_destroy(&log_file_mutex);

    if(file_error_log_descriptor != 0){
        close(file_error_log_descriptor);
    }
    if(file_log_descriptor != 0){
        close(file_log_descriptor);
    }
}

/**
 *  @brief Scrive sul file di error_log un messaggio  passato in input accompagnato dal meaassagio d'errore
 *  relativo al codice d'errore contenuto nella variabile globale errno e dal thread id che la invoca
 *
 * @param msg Messaggio da stampare su file di log
 *
 */

void print_on_error_log(const char msg[200]){

    char tid[TID_MAX_LENGHT];
    snprintf(tid,TID_MAX_LENGHT,"%lu",pthread_self());
    memset(error_log_buffer,'\0',MAX_ERROR_LOG_BUFFER);
    pthread_mutex_lock(&error_log_buffer_mutex);
    write(file_error_log_descriptor,msg,strlen(msg));
    strerror_r(errno, error_log_buffer,150);
    error_log_buffer[strlen(error_log_buffer)]='\0';
    write(file_error_log_descriptor,error_log_buffer,strlen(error_log_buffer));
    write(file_error_log_descriptor,"\ttid: ",6);
    write(file_error_log_descriptor,tid,strlen(tid));
    pthread_mutex_unlock(&error_log_buffer_mutex);

}

/**
 * @brief Scrive sul file di log un messaggio  passato in input accompagnato dal thread id che la invoca
 *
 * @param msg Messaggio da stampare su file di log
 *
 */

void print_on_log(const char msg[200]){

    char tid[TID_MAX_LENGHT];
    snprintf(tid,TID_MAX_LENGHT,"%lu",pthread_self());
    pthread_mutex_lock(&log_file_mutex);
    write(file_log_descriptor,msg,strlen(msg));
    write(file_log_descriptor,"\ttid: ",6);
    write(file_log_descriptor,tid,strlen(tid));
    pthread_mutex_unlock(&log_file_mutex);

}

/**
 * @brief Riscrive in caratteri asci una data temporale
 *
 * @param dateTimeStr Stringa di destinazione
 * @param mytime Varibile temporale da trasformare in stringa
 */

void formatDateTime( char  dateTimeStr[DATE_MAX_LENGHT], const time_t mytime) {


    struct tm date = *localtime(&mytime);//non si potrebbe usare asctime(info) invece dello sprintf
    sprintf(dateTimeStr, "%.2d/%.2d/%4d %.2d:%.2d:%.2d", date.tm_mday, date.tm_mon + 1, date.tm_year + 1900, date.tm_hour, date.tm_min, date.tm_sec);

}