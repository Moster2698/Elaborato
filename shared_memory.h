#ifndef _SHARED_MEMORY_HH
#define _SHARED_MEMORY_HH

#include <stdlib.h>
#include <sys/shm.h>

// Alloca la shared memory data la chiave e la dimensione
int alloc_shared_memory(key_t shmKey, size_t size);

// Ritorna il puntatore alla zona di memoria
void *get_shared_memory(int shmid, int shmflg);

// Rimuove il puntatore alla zona di memoria
void free_shared_memory(void *ptr_sh);

// Elimina la shared memory
void remove_shared_memory(int shmid);

#endif
