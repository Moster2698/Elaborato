#ifndef _SEMAPHORE_HH
#define _SEMAPHORE_HH
#include <errno.h>
// Definizione del sem_un
union my_semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

// Operazioni su semaforo bloccante
void semOp(int semid, unsigned short sem_num, short sem_op);
// Operazioni su semaforo non bloccante
int sem_no_wait(int semid, unsigned short sem_num, short sem_op);
#endif
