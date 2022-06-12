/// @file defines.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche del progetto.

#include "defines.h"
void remove_ipcs()
{
    char *home = getenv("HOME");
    char *fifo1_path, *fifo2_path;
    if (home == (char *)NULL)
        err_exit("getenv failed - ");
    fifo1_path = (char *)malloc(200 * sizeof(char));
    fifo2_path = (char *)malloc(200 * sizeof(char));
    fifo1_path = strcat(strcpy(fifo1_path, home), "/FIFO1");
    fifo2_path = strcat(strcpy(fifo2_path, home), "/FIFO2");
    // rimozione FIFO1 solo se esiste
    if (access(fifo1_path, F_OK) == 0)
    {
        // se la fifo non è aperta non la chiudo
        if (fifo1FD != 0)
        {
            if (close(fifo1FD) == -1)
                err_exit("fifo1 close -");
        }
        unlink(fifo1_path);
    }

    // rimozione FIFO2 sono se esiste
    if (access(fifo2_path, F_OK) == 0)
    {
        // se la fifo non è aperta non la chiudo
        if (fifo2FD != 0)
        {
            if (close(fifo2FD) == -1)
                err_exit("fifo2 close -");
        }
        unlink(fifo2_path);
    }

    // rimozione MsgQueue
    if (msgctl(msgqID, IPC_RMID, NULL) == -1)
        err_exit("MsgQueue removed -");

    // detach della shared memory
    free_shared_memory(p1);

    // eliminazione dell'area condivisa
    remove_shared_memory(shmID);

    // rimuove il set di semafori condiviso tra client e server
    errno = 0;
    if (semctl(semID, 0, IPC_RMID, 0) == -1)
        if (errno != EIDRM)
            err_exit("no semctl -");
    free(fifo1_path);
    free(fifo2_path);
}
