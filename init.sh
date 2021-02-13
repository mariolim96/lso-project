#!/bin/sh

#Questo script ci permette di  compilare  e creare gli eseguibili per tutti i file del progetto
cd Remote-System-Monitor-Client
gcc -o Client *.c && mv Client ../ && cd ..

cd Remote-System-Monitor-Agent

gcc -o Agent *.c && mv Agent ../ && cd ..

cd Remote-System-Monitor-Server

gcc -o Server *.c -pthread && mv Server ../ && cd ..

