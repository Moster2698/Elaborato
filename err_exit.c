/// @file err_exit.c
/// @brief Contiene l'implementazione della funzione di stampa degli errori.

#include "err_exit.h"

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

void err_exit(const char *msg)
{
    perror(msg); // stampa su standard error la stringa che gli viene fornita
    exit(1);
}
