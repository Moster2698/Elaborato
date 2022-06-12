/// @file defines.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del progetto.
#pragma once
#include "semaphore.h"
#include "shared_memory.h"
#include "err_exit.h"
#include "fifo.h"
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sem.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <math.h>
#include <dirent.h>

// ID delle ICP
int msgqID, shmID, semID, fifo1ID, fifo2ID;

// file descriptor
int fifo1FD, fifo2FD;

// puntatore alla shared memory
struct wrapper_message *p1;

// struttura del messaggio
struct message
{
    long mtype;
    char data[1025];
    pid_t pid;
    int id_message;
    char path_name[150];
};

// struttura per la memoria condivisa, verificando l'area occupata
struct wrapper_message
{
    struct message lista_messaggi[50];
    unsigned short indici_messagi[50];
};

// chiavi delle IPCS
#define MSG_KEY 1
#define SHM_KEY 2
#define KEY_SEM 3
#define LOCAL_KEY_SEM 4

// valori utilizzati per la sincronizzazione
#define REQUEST_SENT 0
#define ACK_SERVER_SHARED_MEMORY 1

// funzione di rimozione delle IPCS
void remove_ipcs();