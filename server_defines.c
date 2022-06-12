
#include "server_defines.h"

void create_ipcs()
{
	char *home = getenv("HOME");
	char *fifo1_path, *fifo2_path;
	if (home == (char *)NULL)
		err_exit("getenv failed - ");
	fifo1_path = (char *)malloc(200 * sizeof(char));
	fifo2_path = (char *)malloc(200 * sizeof(char));
	fifo1_path = strcat(strcpy(fifo1_path, home), "/FIFO1");
	fifo2_path = strcat(strcpy(fifo2_path, home), "/FIFO2");
	fifo1FD = 0;
	fifo2FD = 0;
	// IPC: set di due semafori utilizzati per la sincronizzazione
	semID = semget(KEY_SEM, 6, S_IWUSR | S_IRUSR | IPC_CREAT);
	if (semID == -1)
		err_exit("semget failed - ");
	fifo1ID = make_fifo(fifo1_path, S_IRUSR | S_IWUSR | O_NONBLOCK);
	// il server crea la FIFO2
	fifo2ID = make_fifo(fifo2_path, S_IRUSR | S_IWUSR | O_NONBLOCK);
	// il server crea la msgqueue con chiave 1, la dimensione è sufficiente
	msgqID = msgget(MSG_KEY, IPC_CREAT | S_IRUSR | S_IWUSR);
	if (msgqID == -1)
		err_exit("msgqueue failed -");

	// il server crea la shared memory con chiave 2 di dimensione 1025*50 messaggi
	shmID = alloc_shared_memory(SHM_KEY, sizeof(struct wrapper_message));

	// linking della shared memory, in lettura e scrittura
	p1 = get_shared_memory(shmID, 0);

	// recupero il numero di file dalla fifo1
	fifo1FD = open(fifo1_path, O_RDONLY);
	fifo2FD = open(fifo2_path, O_RDONLY);
	if (fifo1FD == -1)
		err_exit("open - ");
	if (fifo2FD == -1)
		err_exit("open - ");
	free(fifo1_path);
	free(fifo2_path);
}