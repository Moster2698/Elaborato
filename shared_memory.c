#include <sys/shm.h>
#include <sys/stat.h>
#include "err_exit.h"
#include "shared_memory.h"
#include <unistd.h>

// funzioane per allocare la shared memory
int alloc_shared_memory(key_t shmKey, size_t size)
{
	int shmid = shmget(shmKey, size, IPC_CREAT | S_IRUSR | S_IWUSR);
	if (shmid == -1)
		err_exit("shmget failed");
	return shmid;
}

// funzione per il linking della shared memory
void *get_shared_memory(int shmid, int shmflg)
{
	void *ptr_sh = shmat(shmid, NULL, shmflg);
	if (ptr_sh == (void *)-1)
		err_exit("shmat failed");
	return ptr_sh;
}

// detach della shared memory
void free_shared_memory(void *ptr_sh)
{
	if (shmdt(ptr_sh) == -1)
		err_exit("shmdt failed");
}

// eliminare la shared memory
void remove_shared_memory(int shmid)
{
	if (shmctl(shmid, IPC_RMID, NULL) == -1)
		err_exit("shmctl failed");
}
