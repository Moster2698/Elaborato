/// @file semaphore.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione dei SEMAFORI.

#include "err_exit.h"
#include "semaphore.h"
#include <sys/sem.h>

// Operazioni su semaforo bloccante
void semOp(int semid, unsigned short sem_num, short sem_op)
{
	struct sembuf sop = {.sem_num = sem_num, .sem_op = sem_op, .sem_flg = 0};

	if (semop(semid, &sop, 1) == -1)
		err_exit("semop failed");
}

// Operazioni su semaforo non bloccante
int sem_no_wait(int semid, unsigned short sem_num, short sem_op)
{
	struct sembuf sop = {.sem_num = sem_num, .sem_op = sem_op, .sem_flg = IPC_NOWAIT};
	errno = 0;
	if (semop(semid, &sop, 1) == -1)
		if (errno != EAGAIN)
			err_exit("semop -");
	/*
	per convenzione il valore di ritorno della funzione è diverso da -1 se non ci sono stati errori
	*/
	if (errno == EAGAIN){
		return -1;
	}else{
		return 1;
	}
}
