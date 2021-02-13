

#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <signal.h>
#include <errno.h>
#include "socketUtility.h"


/*******************************************/

#define UPTIME_MAX_LENGHT 13
#define FREERAM_MAX_LENGHT 12
#define PROCS_MAX_LENGHT 7
#define CONFIRM_MAX_LENGHT 3

/*******************************************/

/**
 * @brief inizializza una socket per predisporre la comunicazione con il server,
 *
 * @param ip_address inidirizzo IP del server al quale ci si vuole connetter
 * @param port  porta da utilizzare per la comunicazione con il server
 */

void init_socket(char * ip_address, unsigned short int port){

    struct sockaddr_in socket_server_addr;

    // Inizializzo la struttura utilizzata per comunicare tramite socket TCP con il server con l'indirizzo ip e la porta passata in input al programma
    memset(&socket_server_addr,'\0', sizeof(socket_server_addr));
    socket_server_addr.sin_family=AF_INET;
    socket_server_addr.sin_addr.s_addr=inet_addr(ip_address);
    socket_server_addr.sin_port=htons(port);

    // Creo la socket TCP per comunicare con il server
    if( (socket_descriptor_agent=socket(PF_INET,SOCK_STREAM,0))< 0 ){
        perror("Errore creazione socket agent: ");
        exit(1);
    }

    // Connetto la socket TCP  alla socket del server
    if( connect(socket_descriptor_agent,(struct sockaddr*)&socket_server_addr, sizeof(socket_server_addr)) != 0){
        perror("Errore connect socket agent: ");
        exit(1);
    }
}

/**
 * @brief Cntrolla se la socket è valida oppure no, quando si entra in comunicazione con il server
 * si riceve un valoore {0,1} che indicano se esiste già un host con questo ip, quindi la socekt
 * va chiusa e il programma deve terminare con un errore, oppure connessione è valida e resta aperta
 *
 * @return  false se il codice ricevuto dal server è 1, true se il codice ricevuto dal server è 0
 */

bool check_socket_validity(){

    ssize_t read_byte;
    char confirm[CONFIRM_MAX_LENGHT];

    // Inizializzo con tutti 0 la stringa confirm
    memset(confirm,'\0',CONFIRM_MAX_LENGHT);

    // Leggo il messaggio di conferma su socket del server per capire se esite già un agent con lo stesso indirizzo ip che sta comunicando con il server
    if((read_byte=read(socket_descriptor_agent,confirm,CONFIRM_MAX_LENGHT)) <= 0){ // Caso in cui la socket lato server è chiusa
        if(read_byte==0){
            write(STDERR_FILENO,"Server non più disponibile",27);
            close(socket_descriptor_agent);
            exit(1);
        }else{
            perror("Errore lettura check validity socket agent: ");
            close(socket_descriptor_agent);
            exit(1);
        }

    }

    confirm[2]='\0';
    // Ritorna il true se il server ha inviato una conferma positiva altrimenti false
    return (strcmp(confirm,"ok")==0) ? true: false;

}

/**
 * @brief  Si occupa di gestire la comunicazione con il server, inviandogli ogni 3 secondi i
 * paramtri uptime, freeram e procs, ricavati con la system call sysinfo().
 * Controlla la validità della socket in caso in cui il server la chiuda immediatamente dopo la sua creazione
 * poichè esiste gia un agent con questo iP che sta comunicando con il server, per evitare ridondanza dei dati
 */

void run_socket_agent(){
    struct sysinfo info_system;
    char uptime[UPTIME_MAX_LENGHT], procs[PROCS_MAX_LENGHT], freeram[FREERAM_MAX_LENGHT];
    char info[UPTIME_MAX_LENGHT+FREERAM_MAX_LENGHT+PROCS_MAX_LENGHT];

    // Inizializzo con tutti 0 le stringhe uptime, freeram e procs
    memset(uptime, '\0',UPTIME_MAX_LENGHT);
    memset(freeram,'\0',FREERAM_MAX_LENGHT);
    memset(procs,'\0',PROCS_MAX_LENGHT);

    // Controllo la validità della socket, ovvero se esiste già un agent che comunica con il server con lo stesso indirizzo ip
    if(check_socket_validity()){
       write(STDOUT_FILENO,"\nLa connessione è avvenuta con successo, Agent Attivo", 54);
       // Ciclo ripetutamente fin tanto che il programma o viene terminato dall'utente oppure è il server a non essere più disponibile
       while(true){
           // Recupero le informazioni sullo stato dell'host tramite la chiamata sysinfo
           if(sysinfo(&info_system)!= 0){
               perror("Errore chiamata di sistema syinfo: ");
               close(socket_descriptor_agent);
               exit(1);
           }

           // Inizializzo con tutti 0 la strina info
           memset(info, '\0', UPTIME_MAX_LENGHT+FREERAM_MAX_LENGHT+PROCS_MAX_LENGHT);
           snprintf(uptime,UPTIME_MAX_LENGHT,"%ld",info_system.uptime);
           snprintf(procs,PROCS_MAX_LENGHT,"%hu",info_system.procs);
           snprintf(freeram,FREERAM_MAX_LENGHT,"%lu",info_system.freeram);
           strcat(info,uptime);
           strcat(info,"\n");
           strcat(info,procs);
           strcat(info,"\n");
           strcat(info,freeram);
           strcat(info,"\0");

           // Comunico le informzioni al server tramite socket TCP
           if (write(socket_descriptor_agent,info,strlen(info)) == -1){ // Caso in cui la socket lato server non è più chiusa
               perror("Errore scrittura socket: ");
               exit(1);
           }
           // Attendi tre secondi prima di rinviare i dati sullo stato dell'host monitorato
           sleep(3);
       }
    }
    write(STDERR_FILENO,"\nEsiste già un processo agent attivo con lo stesso indirizzo IP!", 64);
}