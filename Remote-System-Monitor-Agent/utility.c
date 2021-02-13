

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "utility.h"

/**
 * @brief Data una strmga verifica se ogni suo carattere è un numero o meno
 * @param str Strinag di caratteri da analizzare
 * @return Se la stringa in input src è formata solo da dnumeri restituisci true, altrmenti false
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
 * @brief Data una stringa contenete un indirizzo ip in input, controlla se questo rispetta la forma
 * X.X.X.X dove X è un numero che varia da 0 a 255
 * @param ip Stringa contenente l'indirizzo ip da analizzare
 * @return Ritorna true se la stringa ip rispetta la forma specificata prima, false altrimenti
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

