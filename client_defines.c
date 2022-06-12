#include "client_defines.h"

int contatore = 1;
struct info_files files;

/*-----------------------------------------------------------------------------------------------------
INIZIALIZZA IL CONTENITORE DI FILE CHE SERVE PER L'INVIO DEI MESSAGGI CORRETTI
-----------------------------------------------------------------------------------------------------*/
struct info_files read_all_directories(char *directory_name)
{
	files.number_file = 0;
	for (int i = 0; i < 100; i++)
		files.file_descriptors[i] = 0;
	recursive_dir_read(directory_name);
	return files;
}
/*----------------------------------------------------------------------------------------------------
LEGGE RICORSIVAMENTE TUTTE LE CARTELLE ALLA RICERCA DEI FILE "sendme_"
-----------------------------------------------------------------------------------------------------*/
void recursive_dir_read(char *directory_name)
{
	DIR *current_open_directory;
	struct stat file_attributes;
	if ((current_open_directory = opendir(directory_name)) == NULL)
		err_exit("opendir -");

	struct dirent *current_entry;

	char *nul_position = &directory_name[strlen(directory_name)]; //<--- \0
	// supponiamo di aver preso cartella file /Users/mattia/Desktop/e/Elaborato
	errno = 0;
	while ((current_entry = readdir(current_open_directory)) != NULL)
	{
		// dentro a current_entry ci sono le informazioni di file
		if (current_entry->d_type == DT_DIR &&
			strcmp(current_entry->d_name, ".") != 0 &&
			strcmp(current_entry->d_name, "..") != 0)
		{
			if (directory_name[strlen(directory_name) - 1] != '/')
				strcat(directory_name, "/");
			if (strcat(directory_name, current_entry->d_name) != NULL)
				recursive_dir_read(directory_name);
			else
				err_exit("strcat -");
			*nul_position = '\0';
		}
		else
		{
			// current entry è un file ? Se si, contiene la sringa "sendme_"?
			if (current_entry->d_type == DT_REG &&
				strstr(current_entry->d_name, "sendme_") != NULL &&
				strstr(current_entry->d_name, "_out") == NULL &&
				strstr(current_entry->d_name, ".txt") != NULL)
			{
				char *path = (char *)malloc(strlen(directory_name) + 2 +
											strlen(current_entry->d_name));
				strcpy(path, directory_name);

				if (path[strlen(directory_name) - 1] != '/')
					strcat(path, "/");
				strcat(path, current_entry->d_name);
				strcat(path, "\0");

				if (stat(path, &file_attributes) == -1)
					err_exit("stat failed - ");
				// se il file è <= di 4KB
				if (file_attributes.st_size <= 4096 && file_attributes.st_size >= 4)
				{
					// Apro il file
					int file_descriptor = open(path, O_RDONLY);
					if (file_descriptor == -1)
						err_exit("open failed - ");

					// Salvo il file descriptor del file letto
					files.file_descriptors[files.number_file] = file_descriptor;
					// Salvo la dimensione del file letto
					files.file_dimension[files.number_file] = file_attributes.st_size;
					// Salvo il path del file letto
					strcpy(files.paths[files.number_file], path);
					files.number_file++;
				}
				free(path);
			}
		}
	}
	errno = 0;
	closedir(current_open_directory);
}
/*---------------------------------------------------------------------------------------------------
DIVISIONE DEI FILE E ASSEGNAMENTO DEI QUATTRO MESSAGGI AD OGNI FIGLIO i-ESIMO
----------------------------------------------------------------------------------------------------*/
void file_divider(struct info_files number_files)
{

	struct message messaggi[4];
	int byte_read, buffer_length;
	size_t bytes_left;
	/*
	semaforo utilizzato per far attendere tutti i figli prima di inviare i
	messaggi nelle IPCS
	*/
	int sem_id_divider = semget(LOCAL_KEY_SEM, 1, S_IWUSR | S_IRUSR | IPC_CREAT);
	if (sem_id_divider == -1)
		err_exit("semget failed - ");
	union my_semun arg;
	unsigned short val = number_files.number_file;
	arg.val = val;
	if (semctl(sem_id_divider, 0, SETVAL, arg) == -1)
		err_exit("semctl failed - ");

	/*----------------------------------------------------------------------------------------
	CREAZIONE DEI FIGLI E DIVISIONE DEI FILE
	-----------------------------------------------------------------------------------------*/
	for (int i = 0; i < number_files.number_file; i++)
	{
		bytes_left = number_files.file_dimension[i];
		buffer_length = (int)ceil(number_files.file_dimension[i] / 4.0);
		pid_t pid = fork();
		if (pid == -1)
			err_exit("fork failed - ");
		if (pid == 0)
		{
			// codice dei figli
			for (int times = 0; times < 4; times++)
			{
				// byte letti per read
				byte_read = read(number_files.file_descriptors[i], messaggi[times].data,
								 buffer_length);
				if (byte_read == -1)
					err_exit("read failed - ");
				// aggiorno i byte mancanti da leggere
				bytes_left -= byte_read;
				messaggi[times].data[byte_read] = '\0';
				// inserisco il pid nel messaggio
				messaggi[times].pid = getpid();
				// inserisco l'indice per indicizzare la memoria condivisa
				messaggi[times].id_message = i;
				// aggiungo il path nel messaggio
				strcpy(messaggi[times].path_name, number_files.paths[i]);
				if (times < 3)
					buffer_length = (int)ceil(bytes_left / (3.0 - (double)times));
				fflush(stdout);
			}
			/*-----------------------------------------------------------------------------------------
			I FIGLI ASPETTANO
			------------------------------------------------------------------------------------------*/
			semOp(sem_id_divider, 0, -1);
			semOp(sem_id_divider, 0, 0);
			write_on_ipcs(messaggi);
			close(number_files.file_descriptors[i]);
			exit(0);
		}
	}
	/*--------------------------------------------------------------------------------------------------
	IL PADRE ASPETTA I FIGLI PRIMA DI RIMUOVERE IL SEMAFORO
	---------------------------------------------------------------------------------------------------*/
	while (wait(NULL) > 0);
	if (semctl(sem_id_divider, 0, IPC_RMID, NULL) == -1)
		err_exit("sem_ctl failed  - ");
}
/*-------------------------------------------------------------------------------------------------------
INIZIALIZZAZIONE DELLE IPCS LATO CLIENT (CREATE DAL SERVER)
--------------------------------------------------------------------------------------------------------*/
void init_ipcs()
{
	char *home = getenv("HOME");
	char *fifo1_path, *fifo2_path;
	if (home == (char *)NULL)
		err_exit("getenv failed - ");
	fifo1_path = (char *)malloc(200 * sizeof(char));
	fifo2_path = (char *)malloc(200 * sizeof(char));
	fifo1_path = strcat(strcpy(fifo1_path, home), "/FIFO1");
	fifo2_path = strcat(strcpy(fifo2_path, home), "/FIFO2");
	fifo1FD = open(fifo1_path, O_WRONLY);
	if (fifo1FD == -1)
		err_exit("open FIFO1 failed - ");
	fifo2FD = open(fifo2_path, O_WRONLY);
	if (fifo2FD == -1)
		err_exit("open FIFO2 failed - ");
	// istanziare la shMem che contiene 50 messaggi
	shmID = alloc_shared_memory(SHM_KEY, sizeof(struct wrapper_message));
	p1 = (struct wrapper_message *)get_shared_memory(shmID, 0);
	for (int i = 0; i < 50; i++)
		(p1)->indici_messagi[i] = 0;
	msgqID = msgget(MSG_KEY, IPC_CREAT | S_IRUSR | S_IWUSR);
	if (msgqID == -1)
		err_exit("msgqueue failed -");
	struct msqid_ds ds;
	if (msgctl(msgqID, IPC_STAT, &ds) == -1)
		err_exit("msgctl get dimension ");
	// Change the upper limit on the number of bytes in the mtext // fields of all
	// messages in the message queue to 50 messagges
	ds.msg_qbytes = sizeof(struct message);
	// Update associated data structure in kernel
	if (msgctl(msgqID, IPC_SET, &ds) == -1)
		err_exit("msgctl set dimension ");
	free(fifo1_path);
	free(fifo2_path);
}
/*------------------------------------------------------------------------------------------------------
FUNZIONE VERIFICA L'INVIO DEL MESSAGGIO NELLA CORRISPONDENTE IPC PER OGNI CLIENT
i-ESIMO
-------------------------------------------------------------------------------------------------------*/
int check(int send_check[])
{
	int flag = 1;
	for (int i = 0; i < 4 && flag; i++)
		if (send_check[i] == 1)
			flag = 0;
	return flag;
}
/*------------------------------------------------------------------------------------------------------
SCRITTURA NELLE IPCS
-------------------------------------------------------------------------------------------------------*/
void write_on_ipcs(struct message messaggi[4])
{
	// ogni processo ha il proprio array per gestire l'invio dei frammenti di
	// messaggio
	int check_send[4] = {1, 1, 1, 1};
	errno = 0;
	semOp(semID, 1, -1);
	messaggi[2].mtype = 1;
	semOp(semID, 1, 1);
	// ciclo eseguito fino a che non invio il messaggio
	while (check(check_send) == 0)
	{
		/*------------------------------------------------------------------------------------------------
		INVIO MESSAGGIO 1 NELLA FIFO1
		-------------------------------------------------------------------------------------------------*/
		errno = 0;
		if (check_send[0] == 1 && sem_no_wait(semID, 2, -1) != -1)
		{
			// scrivo e alzo il flag dell'array
			if (write(fifo1FD, &messaggi[0], sizeof(struct message)) == -1)
				err_exit("write on fifo1 -");
			else
				check_send[0] = 0;
		}
		/*------------------------------------------------------------------------------------------------
		INVIO MESSAGGIO 2 NELLA FIFO2
		-------------------------------------------------------------------------------------------------*/
		errno = 0;
		if (check_send[1] == 1 && sem_no_wait(semID, 3, -1) != -1)
		{
			// scrivo e alzo il bit dell'array
			if (write(fifo2FD, &messaggi[1], sizeof(struct message)) == -1)
				err_exit("write on fifo2 -");
			else
				check_send[1] = 0;
		}
		/*------------------------------------------------------------------------------------------------
		INVIO MESSAGGIO 3 NELLA MSGQUEUE
		-------------------------------------------------------------------------------------------------*/
		errno = 0;
		if (check_send[2] == 1 && sem_no_wait(semID, 4, -1) != -1)
		{
			// scrivo e alzo il bit dell'array
			size_t mSize = sizeof(struct message) - sizeof(long);
			if (msgsnd(msgqID, &messaggi[2], mSize, IPC_NOWAIT) == -1)
				if (errno == EAGAIN)
				{
					// il messaggio non può essere inserito nella msgqueue quindi
					// incremento nuovamente il semaforo
					semOp(semID, 4, 1);
				}
				else
					err_exit("write on MSGQUEUE failed -");
			else
				check_send[2] = 0;
		}
		/*------------------------------------------------------------------------------------------------
		INVIO MESSAGGIO 4 NELLA SHMEM IN UNA LOCAZIONE LIBERA
		-------------------------------------------------------------------------------------------------*/
		errno = 0;
		if (check_send[3] == 1 && sem_no_wait(semID, 5, -1) != -1)
		{
			// scrivo e alzo il bit dell'array
			semOp(semID, 1, -1);
			int flag = 0;
			for (int i = 0; i < 50 && flag == 0; i++)
			{
				if ((p1)->indici_messagi[i] == 0)
				{
					(p1)->indici_messagi[i] = 1;
					(p1)->lista_messaggi[i] = messaggi[3];
					flag = 1;
					check_send[3] = 0;
				}
			}
			semOp(semID, 1, 1);
		}
	}
}
