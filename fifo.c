/// @file fifo.c
/// @brief Contiene l'implementazione delle funzioni
///         specifiche per la gestione delle FIFO.

#include "err_exit.h"
#include "fifo.h"
#include <sys/stat.h>
#include <sys/types.h>

// crea la fifo
int make_fifo(char *name, int flags)
{
    int id = mkfifo(name, flags);
    if (id == -1)
        err_exit("fifo -");
    return id;
}
