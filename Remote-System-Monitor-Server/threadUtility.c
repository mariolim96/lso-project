
#include "threadUtility.h"
#include "server.h"
#include <pthread.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

/*****************************************************/

#define UPTIME_MAX_LENGHT 13
#define PROCS_MAX_LENGHT 7
#define FREERAM_MAX_LENGHT 12
#define TYPE_COMUNICATION_MAX_LENGHT 2
#define CONFIRM_MAX_LENGHT 3
#define REQUEST_MAX_LENGHT 2
#define IP_ADDRESS_MAX_LENGHT 16
#define HOSTNAME_MAX_LENGHT 257
#define INT_TO_ASCI_MAX_LENGHT 7

#define REQUEST_HOSTS_LIST 1
#define REQUEST_HOST_INFORMATION 2
#define REQUEST_EXIT 3

#define SOCKET_CLOSED -1

/*****************************************************/

agent_thread_utility_t agentThreadUtility;
struct timespec timespec;

/*****************************************************/


/**
 * @brief Parserizza i dati contenuti nella stringa src, dividendo i dati ottenuti
 * in tre paramtri :uptime,freeram, procs.
 * La struttura della stringa src è convenzionalmente sempre la seguente:
 * [uptime]
 * [procs]
 * [freeram]
 *
 *
 * @param src     stringa contenente i dati letti da una socket
 * @param uptime  stringa in cui sarà salvato il valore uptime, contenuto in src
 * @param freeram stringa in cui sarà salvato il valore freeram, contenuto in src
 * @param procs   stringa in cui sarà salvato il valore procs, contenuto in src
 */

void formact_data(const char *src, char *uptime, char *freeram, char *procs){

    if(src==NULL){
        memset(uptime,'\0',UPTIME_MAX_LENGHT);
        memset(freeram,'\0',FREERAM_MAX_LENGHT);
        memset(procs,'\0',PROCS_MAX_LENGHT);
    }

    int i=0;
    int uptime_index,freeram_index,procs_index;
    uptime_index=freeram_index=procs_index=0;

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

/**
 * @brief Funzione che viene eseguita dal thread che si occupa di comunicare con un singolo agent,
 * questo thread quindi si occupa di ricavare:
 * l'indirizzo ip e l'hostname dell'agent con cui sta comunicando, freeram, uptime e procs comunicatogli
 * dall'agent tramite la socket, ed impostare tutti questi dati insieme alla data corrente all'interno della
 * struttura connected_hosts, per l'host avente indirizzo ip uguale all'agent con cui sta comunicando.
 * Questa funzione prima di aggingere un nuovo host alla struttura connected_hosts, controlla se c'è spazione,
 * in caso negativo raddoppia la dimensione della struttura e rialloca lo spazio corretto.
 * Controlla se l'agent con cui si sta comunicando  era già presente nella struttura connected_host, in caso positivo
 * va aggiornare i dati precedenti, mentre in caso negativo aggiunge dati alla struttura
 *
 * @param arg Contiente una struttura agent_thread_utility_t alla quale viene effettuato il cast a (void *), questo
 * campo contiene il socket_descriptor della socket da utilizzare per la comunicazione con l'agent ,un flag che indica
 * se quell'agent è già presente nella struttura connected_hosts, e la posizione da utilizzare all'interno della struttura
 * connected_hosts.
 *
 * @return Non ritorna niente dato che questo thread sarà utilizzato in modalità detached
 */

void *agent_thread(void *arg){

    struct sockaddr_in new_socket_agent_addr;
    unsigned int len = sizeof(new_socket_agent_addr);
    char buffer[UPTIME_MAX_LENGHT+PROCS_MAX_LENGHT+FREERAM_MAX_LENGHT],uptime[UPTIME_MAX_LENGHT],procs[PROCS_MAX_LENGHT],freeram[FREERAM_MAX_LENGHT];
    char *hostname=calloc(HOSTNAME_MAX_LENGHT, sizeof(char));
    assert(hostname !=0);
    char *ip_address;
    ssize_t read_byte;
    long uptime_t;
    unsigned long freeram_t;
    unsigned  short procs_t;

    print_on_log("\nNuovo Agent connesso\tsarà servito dal thread");


    // Blocco il mutex della variabile globale agentThreadUtility assicurandomi di averne esclusivamente il controllo in modo, salvo i dati in esso contenuti in varibili locali e ne rilascio il controllo
    // in questo modo questa varibaile è pronta per essere utilizzata da altri thread che si stanno connettendo
    pthread_mutex_lock(&agentThreadUtility.mutex);
    const int socket_desciptor=agentThreadUtility.socket_descriptor;
    const bool flag=agentThreadUtility.flag;
    pthread_mutex_lock(&connected_hosts.mutex);
    const unsigned short int id=(flag) ? agentThreadUtility.next_index : connected_hosts.next_index;
    pthread_mutex_unlock(&agentThreadUtility.mutex);

    // Ricavo le informazioni sul host con cui sto comunicando tramite socket e le salvo nella struttutra new_socket_agent
    if (getpeername(socket_desciptor,(struct sockaddr*)&new_socket_agent_addr,&len)!= 0){
        print_on_error_log("\nErrore getpeername socket agent: ");
        exit(1);
    }

    // Ottengo l'indirizzo ip dell'host con cui sto comunicando
    ip_address=inet_ntoa(new_socket_agent_addr.sin_addr);

    // Ottengo l'hostname dell'host con cui sto comunicando
    if(getnameinfo((struct sockaddr*)&new_socket_agent_addr,len,hostname,HOSTNAME_MAX_LENGHT,NULL,0,0)!=0){
        print_on_error_log("\nErrore getname info socket agent: ");
        exit(1);
    }

    if(!flag){  // Caso in cui nella struttura connected_hosts non è presente già un hosts con questo indirizzo ip

        // Alloco tutta la memoria necessaria per contere le informazioni del nuovo host, queste vengono salvate in una struttura di tipo host_t
        connected_hosts.hosts[id]=(host_t*)malloc(sizeof(host_t));
        assert(connected_hosts.hosts[id]!=0);
        connected_hosts.hosts[id]->ip_address=calloc((strlen(ip_address)+1), sizeof(char));
        assert(connected_hosts.hosts[id]->ip_address!=0);
        connected_hosts.hosts[id]->hostname=calloc((strlen(hostname)+1), sizeof(char));
        assert(connected_hosts.hosts[id]->hostname!=0);
        pthread_mutex_init(&connected_hosts.hosts[id]->mutex,NULL);
        // Assumo il controllo della struttura host appena create e gli assegno tutti i valori corretti dell'host connesso
        pthread_mutex_lock(&connected_hosts.hosts[id]->mutex);
        time(&connected_hosts.hosts[id]->last_communication);
        connected_hosts.hosts[id]->isActive=true;
        strcpy(connected_hosts.hosts[id]->ip_address,ip_address);
        connected_hosts.hosts[id]->ip_address[strlen(ip_address)]='\0';
        strcpy(connected_hosts.hosts[id]->hostname,hostname);
        connected_hosts.hosts[id]->hostname[strlen(hostname)]='\0';
        connected_hosts.next_index++;
        // Sblocco la risorsa
        pthread_mutex_unlock(&connected_hosts.hosts[id]->mutex);

        // Controllo se la struttura connected_host è piena, in questo caso raddoppio la sua capacità e alloco nuovo spazio
        if(connected_hosts.next_index >= connected_hosts.dim){
            short unsigned prec_dim=connected_hosts.dim;
            connected_hosts.dim = connected_hosts.dim*2;
            connected_hosts.hosts=realloc(connected_hosts.hosts, connected_hosts.dim*sizeof(host_t**));
            assert(connected_hosts.hosts!=0);
            for (int i = prec_dim; i < connected_hosts.dim; i++) {
                connected_hosts.hosts[i]=NULL;
            }
        }
        // Dealloco la stringa hostname in quanto non mi servirà più
        free(hostname);
    }

    // Sblocco la struttura connected_hosts
    pthread_mutex_unlock(&connected_hosts.mutex);

    while (true) {
        //Inizializzo con tutti zero la stringa buffer
        memset(buffer,'\0',UPTIME_MAX_LENGHT+FREERAM_MAX_LENGHT+PROCS_MAX_LENGHT);
        // Leggo da socket TCP le informazioni comunicate dall'agent relativo all'host
        if ((read_byte = read(socket_desciptor, buffer, UPTIME_MAX_LENGHT+FREERAM_MAX_LENGHT+PROCS_MAX_LENGHT)) <= 0) {
            if (read_byte == 0) {            // La comunicazione con l'agent ha esito negativo in quanto la socket lato agent è chiusa
                close(socket_desciptor);
                print_on_log("\nIl thread agent è terminato, socket lato agent risulta chiusa");
                pthread_exit(NULL);
            } else {
                print_on_error_log("\nErrore lettura dati da socket agent: ");
                exit(1);
            }
        }

        // Parserizzo i dati ricevuti sa socket  e li salvo in tre stringhe uptime,freeram e procs, in seguito le converto nei relativi valori numerici
        formact_data(buffer, uptime, freeram, procs);
        uptime_t = strtol(uptime, NULL, 10);
        freeram_t = strtoul(freeram, NULL, 10);
        procs_t = strtoul(procs, NULL, 10);
        // Assumo di nuovo il controllo della struttura connected_hosts, assegno i valori relativi all'host appena ricevute dall'agent e rilascio la risorsa
        pthread_mutex_lock(&connected_hosts.hosts[id]->mutex);
        connected_hosts.hosts[id]->uptime=uptime_t;
        connected_hosts.hosts[id]->free_ram=freeram_t;
        connected_hosts.hosts[id]->procs=procs_t;
        time(&connected_hosts.hosts[id]->last_communication);
        pthread_mutex_unlock(&connected_hosts.hosts[id]->mutex);
    }

}

/**
 * @brief Funzione che viene eseguita dal thread che gestisce la socket di comunicazione con gli agent
 * che tentano la connessione al server, per ogni richiesta che perviene controlla se l'indirizzo ip dell'agent
 * è già presente nella struttura connected_hosts o meno, nel caso in cui non sia presente istituisce un nuovo thread
 * che si occupera di comunicare con quel agent, in caso questo era già presente controlla il suo stato di attività, così
 * da creare un nuovo thread solo nel caso in cui quel host era nello stato di inattività riutilizzando la struttura precedentemente
 * a lui assegnata sovrascrivendo i dati precedenti
 *
 * @param arg Non utilizzato
 * @return  Non ritorna niente dato che questo thread sarà utilizzato in modalità detached
 */

void *agent_controller_thread(void *arg){

    struct sockaddr_in new_socket_agent_addr;
    unsigned int len = sizeof(new_socket_agent_addr);
    int new_socket_agent_descriptor;
    pthread_t new_pthread_agent;
    char *ip_address;
    bool host_already_present=false;

    // Inizializzo la struttra che utilizzero per salvare le informazioni utili alla connessione di un nuovo host con tutti 0
    memset(&new_socket_agent_addr, '\0', sizeof(new_socket_agent_addr));
    // Inizializzo il mutex della variabile globale agentThreadUtility in modo da renderla thread safe
    pthread_mutex_init(&agentThreadUtility.mutex,NULL);

    while (true) {
        // Accetto la connessione alla socket usata per comunicare gli agent di un nuovo host
        if ((new_socket_agent_descriptor = accept(socket_agent_descriptor_controller,(struct sockaddr *) &new_socket_agent_addr, &len)) < 0) {
            print_on_error_log("\nErrore accept agent controller: ");
            exit(1);
        }

        // Ricavo le informazioni dell'indirizzo del nuovo host connesso e setto le informazioni della variabile globale agentThreadUtility con quelle relative al nuovo host
        ip_address=inet_ntoa(new_socket_agent_addr.sin_addr);//network to ip4
        // provo ad ottenere il controllo sulla struttuta agentThreadUtility
        pthread_mutex_lock(&agentThreadUtility.mutex);
         gentThreadUtility.socket_descriptor=new_socket_agent_descriptor;
        agentThreadUtility.flag=false;

        // Blocco la struttura connected_hosts affinche possa effettuare una ricerca al suo interno in modo sicuro e coerente
        pthread_mutex_lock(&connected_hosts.mutex);

        // Se la struttura connected_hosts è vuota posso essere sicuro che il nuovo host non sia già presente all'interno dei dati salvati sul server
        if(connected_hosts.next_index==0){
            host_already_present=false;
        } else{
            // Effettuo l a ricerca all'interno della struttura connected_hosts aggiornando il flag host_already_present solo nel caso in cui trovo un host con lo stesso ip e in uno stati dfi inattività da piú di 6 secondi
            for (int i = 0; i <connected_hosts.next_index ; i++) {
                if(strcmp(connected_hosts.hosts[i]->ip_address,ip_address)==0){
                    // L'accesso all'host con indirizzo ip cercato e thread safe in quanto blocco il relativo mutex e lo sblocco un volta terminato
                    pthread_mutex_lock(&connected_hosts.hosts[i]->mutex);
                    host_already_present=(connected_hosts.hosts[i]->isActive) ? true : false;
                    // In questo caso devo utilizzare all'interno della struttura connected_hosts riutilizzo la precedente locazione, andando  solamente ad aggiornare i dati senza crearne dei nuovi
                    agentThreadUtility.next_index=i;
                    agentThreadUtility.flag=true;
                    pthread_mutex_unlock(&connected_hosts.hosts[i]->mutex);
                    break;
                }
            }
        }

        // Sblocco la struttura connected_hos
        pthread_mutex_unlock(&connected_hosts.mutex);

        // Sblocco la struttura agentThreadUtility
        pthread_mutex_unlock(&agentThreadUtility.mutex);

        if(!host_already_present){    // Caso in cui il nuovo host collegato non è già presente nella struttura connected_hosts

            // Procedo a confermare all'agent una risposta positiva tramite socket
            if(write(new_socket_agent_descriptor, "ok", CONFIRM_MAX_LENGHT) == -1){ 
                // La comunicazione con l'agent ha esito negativo in quanto la socket lato agent è chiusa
                print_on_error_log("\nErrore scrittura conferma nuovo agent: ");
            } else{
                // Creo un nuovo thread che da ora in poi si occupera di gestire la comunicazione con quel host, gli passo come argomento la variabile gloabale agentThreadUtility che in questo momemento contiene
                // le informazioni su quale sarà la sua posizione all'interno della struttura connected_hosts e il socket descriptor che utilizzerà per comunicare con l'host
                if (pthread_create(&new_pthread_agent, NULL, agent_thread, (void *)&agentThreadUtility) < 0) {
                    print_on_error_log("\nErrore creazione thread agent: ");
                    exit(1);
                }

                // Imposto lo stato del nuovo thread a detached
                if (pthread_detach(new_pthread_agent) != 0) {
                    print_on_error_log("\nErrore set detach state thread agent: ");
                    exit(1);
                }
            }

        } else{// Caso in cui il nuovo host collegato  è già presente nella struttura connected_hosts
            // Procedo a confermare all'agent una risposta negativa tramite socket in modo da terminare il programma agent immediatamente
            if(write(new_socket_agent_descriptor, "no", CONFIRM_MAX_LENGHT) == -1){  // La comunicazione con l'agent ha esito negativo in quanto la socket lato agent è chiusa
                print_on_error_log("\nErrore scittura conferma nuovo agent: ");
            }
        }
    }

}

/**
 *@brief Dato un Socket descriptor da cui attingere le informazioni, legge la richiesta
 * ricevuta e ne ritorna il valore intero associato
 *
 * @param socket_descriptor Socket descriptor da cui ci si aspetta di leggere le informazioni
 * @return Ritorna il valore della richiesta, contenuto in una stringa sotto forma di short int, oppure  in caso di lettura
 * da una socket chiusa restituisce SOCKET_CLOSED , ovvero un errore!
 *
 */

short int read_request(int socket_descriptor){

    char buffer[REQUEST_MAX_LENGHT];
    ssize_t read_byte;
    char *ris=NULL;

    //Inizializzo con tutti zero la stringa buffer
    memset(buffer,'\0',REQUEST_MAX_LENGHT);

    //Leggo da socket la richiesta di comando da eseguire comunicata dal client
    if( (read_byte=read(socket_descriptor,buffer,REQUEST_MAX_LENGHT)) <=0 ){
        if(read_byte==0){
            return SOCKET_CLOSED;
        }else{
            print_on_error_log("\nErrore lettura richiesta client: ");
            exit(1);
        }
    }

    buffer[1]='\0';
    // Ritorno il valore numerico relativo alla stringa letta da socket
    return (short)strtol(buffer, &ris, 10);

}

/**
 * @brief Comunica tramite una socket  il numero di host attualmente connessi al server, ovvero
 * salvati nella struttura connected_hosts
 *
 * @param socket_descriptor  Socket descriptor da cui ci si aspetta di leggere le informazioni
 * @return Ritorna il numero di host connessi al server, oppure  in caso di lettura da una socket chiusa restituisce SOCKET_CLOSED , ovvero un errore!
 *
 */

short int  comunication_number_host_connected(int socket_descriptor){

     char comunication_number_host[INT_TO_ASCI_MAX_LENGHT];
     char confirm[CONFIRM_MAX_LENGHT];
     short unsigned number_host;
     ssize_t read_byte;

    //Inizializzo con tutti zero le stringhe comunication_number_host e confirm
     memset(comunication_number_host,'\0',INT_TO_ASCI_MAX_LENGHT);
     memset(confirm,'\0',CONFIRM_MAX_LENGHT);

     // Blocco la struttura connected_hosts e ricavo l'informazione su quanti sono gli host salvati nel server e poi ne rilascio il controllo
     pthread_mutex_lock(&connected_hosts.mutex);
     number_host=connected_hosts.next_index;
     sprintf(comunication_number_host, "%hu",number_host);
     comunication_number_host[strlen(comunication_number_host)]='\0';
     pthread_mutex_unlock(&connected_hosts.mutex);

     nanosleep(&timespec,NULL);

     // Comunico al client tramite socket il  numero di host attualmenti connessi al server
     if( write(socket_descriptor,comunication_number_host,strlen(comunication_number_host)) == -1 ){
         return SOCKET_CLOSED;
     }

     // Leggo da socket un messaggio di conferma da parte del client
     if( (read_byte=read(socket_descriptor,confirm,CONFIRM_MAX_LENGHT)) <= 0 ){
        if(read_byte==0){ // La comunicazione con l'agent ha esito negativo in quanto la socket lato agent è chiusa
            return SOCKET_CLOSED;
        } else{
            print_on_error_log("\nErrore lettura conferma client: ");
            exit(1);
        }
     }

     return (short)number_host;

}

 /**
  * @brief Comunica la lista di tutti gli host attualmente connessi al server, contenuti nella struttura
  * connected_hosts
  *
  * @param socket_descriptor Socket descriptor utilizzato per comunicare
  * @return ritorna 0 in caso di avvenuta comunicazione, oppure SOCKET_CLOSED in caso di errore
  */

 short int comunication_hosts_list(int socket_descriptor){

     char comunication_hosts_list[IP_ADDRESS_MAX_LENGHT+HOSTNAME_MAX_LENGHT];
     short int number_host;

     // Comunico al client il numero di host connessi al server
     number_host = comunication_number_host_connected(socket_descriptor);

     if(number_host == SOCKET_CLOSED){  // La comunicazione con il client ha esito negativo in quanto la socket lato client risulta chiusa
         return SOCKET_CLOSED;
     }

     for (int i = 0; i <number_host ; i++) {

         // Inizializzo con tutti zero le stringhe comunication_hosts_list
          memset(comunication_hosts_list,'\0', IP_ADDRESS_MAX_LENGHT+HOSTNAME_MAX_LENGHT);
          // Blocco la struttura host e mi salvo i dati contenuti al suo interno in una stringa comunication_hosts_list e la sblocco
          pthread_mutex_lock(&connected_hosts.hosts[i]->mutex);
          strcpy(comunication_hosts_list,connected_hosts.hosts[i]->ip_address );
          strcat(comunication_hosts_list,"\n");
          strcat(comunication_hosts_list,connected_hosts.hosts[i]->hostname);
          comunication_hosts_list[strlen(comunication_hosts_list)]='\0';
          pthread_mutex_unlock(&connected_hosts.hosts[i]->mutex);

          nanosleep(&timespec,NULL);
          //Comunico tramite socket al client le informazioni ip e hostname su un host connesso al client
          if ( write(socket_descriptor,comunication_hosts_list,strlen(comunication_hosts_list)) == -1){ // La comunicazione con il client ha esito negativo in quanto la socket lato client è chiusa
             return SOCKET_CLOSED;
          }
     }

     return 0;

}

 /**
  * @brief Legge da socket l'indice dell'host di cui si vuole conoscere le informazioni
  *
  * @param socket_descriptor Socket descriptor utilizzato per la comunicazione
  * @return Ritorna l'indice corretto oppure in caso di errore SOCKET_CLOSED
  *
  */

short int read_index_host(int socket_descriptor){

     char index_host[INT_TO_ASCI_MAX_LENGHT];
     ssize_t read_byte;

     // Inizializzo con tutti 0 la stringa index
     memset(index_host,'\0',INT_TO_ASCI_MAX_LENGHT);

     // Leggo tramite socket dal client l'indice dell'host che si è scelto per conoscerne le informazioni sullo stato
     if( (read_byte=read(socket_descriptor,index_host,INT_TO_ASCI_MAX_LENGHT))<=0){
        if(read_byte==0){
            return SOCKET_CLOSED;
        } else {
            print_on_error_log("\nErrore lettura indice client: ");
            exit(1);
        }
     }

     index_host[strlen(index_host)]='\0';
     // Ritorno il rispettivo valore numerico dell'indice scelto letto da socket
     return (short)strtoul(index_host,NULL,10);

}

/**
 * @brief Comunica le informazioni di un singolo host contenute nella struttura connected_hosts tramite una socket
 *
 * @param socket_descriptor Socket descriptor utilizzato per la comunicazione
 * @return ritorna 0 in caso di avvenuta comunicazione, oppure SOCKET_CLOSED in caso di errore
 */

short int comunication_host_information(int socket_descriptor){

    char uptime[UPTIME_MAX_LENGHT], procs[PROCS_MAX_LENGHT], freeram[FREERAM_MAX_LENGHT];
    char comunication[UPTIME_MAX_LENGHT+FREERAM_MAX_LENGHT+PROCS_MAX_LENGHT];
    char confirm[CONFIRM_MAX_LENGHT];
    char type_comunication[TYPE_COMUNICATION_MAX_LENGHT];
    char date[DATE_MAX_LENGHT];
    short index_host_val;
    ssize_t read_byte;

    // Leggo l'indice dell'host scelto dal client
    index_host_val=read_index_host(socket_descriptor);


    if(index_host_val == SOCKET_CLOSED){        // Caso in cui la socket lato client è chiusa
        return SOCKET_CLOSED;
    }

    // Inizializzo a 0 le stringe comunication, type_comunication, confirm e date
    memset(comunication, '\0',UPTIME_MAX_LENGHT+FREERAM_MAX_LENGHT+PROCS_MAX_LENGHT);
    memset(type_comunication,'\0',TYPE_COMUNICATION_MAX_LENGHT);
    memset(confirm, '\0',CONFIRM_MAX_LENGHT);
    memset(date,'\0',DATE_MAX_LENGHT);

    // Blocco la struttura connected_hosts
    pthread_mutex_lock(&connected_hosts.hosts[index_host_val]->mutex);


   if(connected_hosts.hosts[index_host_val]->isActive){ // Caso in cui sto comunicando dati di un host atttivo al client, ovvero  un host che ha aggiornato i suoi dati negli ultimi 6 secondi

      strcpy(type_comunication,"1");
      nanosleep(&timespec,NULL);

      // Comunico al client quindi in che modo deve interpretare la comunicazione se come un host attivo o come un host inattivo
      if ( write(socket_descriptor,type_comunication,strlen(type_comunication)) == -1){      // Caso in cui la socket lato client è chiusa
          return SOCKET_CLOSED;
      }

      //Leggo da socket un messaggio di conferma di dati ricevuti dal client
      if( (read_byte=read(socket_descriptor,confirm,CONFIRM_MAX_LENGHT)) <= 0){  // Caso in cui la socket lato client è chiusa
          if(read_byte==0){
              return SOCKET_CLOSED;
          } else{
              print_on_error_log("\nErrore lettura conferma client: ");
              exit(1);
          }
      }

      snprintf(uptime,UPTIME_MAX_LENGHT,"%ld",connected_hosts.hosts[index_host_val]->uptime);
      snprintf(procs,PROCS_MAX_LENGHT,"%hu",connected_hosts.hosts[index_host_val]->procs);
      snprintf(freeram,FREERAM_MAX_LENGHT,"%lu",connected_hosts.hosts[index_host_val]->free_ram);
      // Sblocco la struttura connected_hots
      pthread_mutex_unlock(&connected_hosts.hosts[index_host_val]->mutex);
      strcpy(comunication,uptime);
      strcat(comunication,"\n");
      strcat(comunication,procs);
      strcat(comunication,"\n");
      strcat(comunication,freeram);
      comunication[strlen(comunication)]='\0';
      nanosleep(&timespec,NULL);

      // Comunico i dati al client
      if ( write(socket_descriptor,comunication,strlen(comunication)) == -1){    // Caso in cui la socket lato client è chiusa
          return SOCKET_CLOSED;
      }

   }else{       // Caso in cui sto comunicando dati di un host inattivo al client, ovvero  un host che non ha aggiornato i suoi dati negli ultimi 6 secondi
       strcpy(type_comunication,"0");
       nanosleep(&timespec,NULL);

       // Comunico al client quindi in che modo deve interpretare la comunicazione se come un host attivo o come un host inattivo
       if (write(socket_descriptor,type_comunication,strlen(type_comunication)) == -1){ // Caso in cui la socket lato client è chiusa
          return SOCKET_CLOSED;
       }

       // Leggo da socket un messaggio di conferma di dati ricevuti dal client
       if( (read_byte=read(socket_descriptor,confirm,CONFIRM_MAX_LENGHT))<=0){   // Caso in cui la socket lato client è chiusa
          if(read_byte==0){
              return SOCKET_CLOSED;
          } else{
              print_on_error_log("\nErrore lettura conferma client: ");
              exit(1);
          }
       }

       // Converto la data che indica l'ultima comunicazione con un host in una stringa
       formatDateTime(date,connected_hosts.hosts[index_host_val]->last_communication);
       // Sblocco la struttura connected_hots
       pthread_mutex_unlock(&connected_hosts.hosts[index_host_val]->mutex);
       nanosleep(&timespec,NULL);

       // Comunico i dati al client
       if ( write(socket_descriptor,date,strlen(date)) == -1){   // Caso in cui la socket lato client è chiusa
          return SOCKET_CLOSED;
       }

   }
   return 0;
}

/**
 * @brief unzione che viene eseguita dal thread che si occupa di comunicare con un singolo client,
 * questo thread quindi si occupa di attendere una richiesta dal client,in base ad una delle tre possibili richieste ottenute
 * dal client eseguire un operazione congrua, le operazioni possibili sono:
 * comunicare la lista degli host connessi al server attualmente, comunicare le informzioni di un host scelto dal client
 * oppure terminare la comunicazione con il cliente quindi anche questo thread che non avrebbe senso piu di esistere
 * @param arg Contiene il socket descriptor da utilizzare per comunicare  con il client
 * @return Non ritorna niente dato che questo thread viene utilizzato in stato detached, quindi la sua terminazione è legata alla terminzaione di del server
 */

void *client_thread(void *arg){

    int socket_descriptor=*(int*)arg;
    short int request;

    print_on_log("\nNuovo client connesso\tsarà servido dal thread");

    // ciclo fin a quando la comunicazione con il client è attiva, ovvero fin a quando  il client non termina la sua esecuzione
    while (true){
        // Leggo la richiesta del client, questa richiesta può avere solo tre possibili valori definiti dalle costanti REQUEST_HOSTS_LIST, REQUEST_HOST_INFORMATION e REQUEST_EXIT 3
        if( ( request = read_request(socket_descriptor)) == SOCKET_CLOSED){    // Caso in cui la socket lato client risulta chiusa
            print_on_log("\nIl thread client è terminato, socket lato client risulta chiusa");
            free(arg);
            pthread_exit(NULL);
        } else{
            switch (request) {
                case REQUEST_HOSTS_LIST:{
                    print_on_log("\nInvio richiesta REQUEST_HOSTS_LIST al server");
                    // Comunico al client la lista di tutti gli host connessi al server
                    if( comunication_hosts_list(socket_descriptor)!=0 ){    // Caso in cui la socket lato client risulta chiusa
                        print_on_log("\nIl thread client è terminato, socket lato client risulta chiusa");
                        free(arg);
                        pthread_exit(NULL);
                    }
                }
                    break;
                case REQUEST_HOST_INFORMATION:{
                    print_on_log("\nInvio richiesta REQUEST_HOST_INFORMATION al server");
                    // Comunico al client le informazioni su un host scelto dal client
                   if( comunication_host_information(socket_descriptor) != 0){   // Caso in cui la socket lato client risulta chiusa
                       print_on_log("\nIl thread client è terminato, socket lato client risulta chiusa");
                       free(arg);
                       pthread_exit(NULL);
                    }
                }
                    break;
                case REQUEST_EXIT:{
                    print_on_log("\nInvio richiesta REQUEST_EXIT al server");
                    print_on_log("\nIl thread client è terminato");
                    // Chiusa la connessione volutamente dal  client
                    free(arg);
                    pthread_exit(NULL);
                }
                default:{

                }
            }
        }
    }

}

/**
 * @brief Gestisce la socket utilizzata per comunicare con i client, ad ogni nuova connessione creata
 * crea un nuovo thread al quale delegare la comunicazione con il client, restando disponibile per accettare
 * nuove connessioni non bloccando la comunicazione client server
 * Imposta i valori di attesa della struttura timespec ad un valore predefinito, utilizzato per sicnronizzare
 * a livello applicativo la comunicazione tra client e server.
 *
 *
 * @param arg Non utilizzato
 * @return Non ritorna niente dato che questo thread viene utilizzato in stato detached, quindi la sua terminazione è legata alla terminzaione di del server
 */

void *client_controller_thread(void *arg){

    struct sockaddr_in new_socket_client_addr;
    int new_socket_client_descriptor;
    pthread_t new_pthread_client;
    unsigned int len = sizeof(new_socket_client_addr);
    int *arg_thred=NULL;

    // Inizializzo la struttura temporale utilizzata per sincronizzare le comunicazioni tra client e server, ed inizializzo con tutti 0 la struttura new_socket_client_addr
    timespec.tv_sec=0;
    timespec.tv_nsec=100000000;
    memset(&new_socket_client_addr, '\0', sizeof(new_socket_client_addr));

    while (true) {
        // Accetto una nuoca connessione
        if ((new_socket_client_descriptor = accept(socket_client_descriptor_controller,(struct sockaddr *) &new_socket_client_addr, &len)) < 0) {
            print_on_error_log("\nErrore accept client controller: ");
            exit(1);
        }
        // alloco spazio per un intero, il quale conterrà il nuovo socket descritor che utilizzerà un nuovo thread per comunicare con il client, sarà questo thread a deallocare questa risorsa dato che sarà l'unico a poterne accedere
        arg_thred=malloc(sizeof(int));
        assert(arg_thred != 0);
        *arg_thred=new_socket_client_descriptor;

        // Creo il nuovo thread che si occuperà di comunicare con il client
        if (pthread_create(&new_pthread_client, NULL, client_thread,(void*)arg_thred) < 0) {
               print_on_error_log("\nErrore creazione thread client controller: ");
                exit(1);
        }

        // Imposto lo stato del nuovo thread a detached
        if (pthread_detach(new_pthread_client) != 0) {
               print_on_error_log("\nErrore set detach state thread client controller: ");
               exit(1);
        }
    }
}

/**
 * @brief Funzione eseguita dal thread che si occupa di controllare periodicamente la struttura connected_hosts
 * aggiornando lo stato di attività di ogni host presente,contrassegndolo come attivo se ha inviato una comunizaione
 * entro 6 secondi oppure come inattivo se l'ultima comunicazione risiede a più di 6 secondi dalla data corrente
 * questo thread è di tipo detached, dao che la sua terminazione è legata alla terminazione del server
 * @param arg Non utilizzato
 * @return Non utilizzato
 */

void *periodic_update_thread(void *arg){

    time_t current_time;
    unsigned short current_dim;
    double time_difference;

    // Ciclo sistematicamente in modo da ripetere queste operazioni fino a quando non termina il server
    while(true){
        // Aggiorna la variabile current_time con l'ora esatta di sistema
        if ( time(&current_time) != ((time_t)-1)){  // Caso in cui la chiamata di sistema time fallisce

            // Blocco la struttura connected_hosts, salvo l'informzione sul numero attuale di host connessi al server e la rilascio
            pthread_mutex_lock(&connected_hosts.mutex);
            current_dim=connected_hosts.next_index;
            pthread_mutex_unlock(&connected_hosts.mutex);

            // Per ogni host connesso al server, blocco quella risorsa, calcolo la differnza di tempo tra la data attuale è quella dell'ultima comunicazione con quel host,
            // se la differenza temporale è maggiore di 6 secondi allor aimposto quel host come inattivo, in fine relascio quella risorsa e procedo fin tanto che non ho analizzato
            // tutti gli host presenti nella struttura connected_hosts
            for (int i = 0; i <current_dim ; i++) {
                pthread_mutex_lock(&connected_hosts.hosts[i]->mutex);
                time_difference=difftime(connected_hosts.hosts[i]->last_communication,current_time);
                connected_hosts.hosts[i]->isActive=(time_difference  > -6) ? true : false;
                pthread_mutex_unlock(&connected_hosts.hosts[i]->mutex);
            }

        } else {
            print_on_error_log("\nErrore chiamata di sistema time(): ");
            exit(1);
        }

        // attendo 6 secondi per ripetere i controlli
        sleep(6);
    }

}



/**
 * @brief Permette di rilasciare ogni risorsa allocata dinamicamente dai thread
 *
 */

void all_resource_thread_relase(){

    // Disruggo il mutex utilizzato dalla struttura globale agentThreadUtility
    pthread_mutex_destroy(&agentThreadUtility.mutex);

}