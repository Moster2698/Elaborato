/// @file sender_manager.c
/// @brief Contiene l'implementazione del sender_manager.

#include "server_defines.h"
/*
Handler che ricevuto un SIGINT invia un SIGUSR1 al client_0,
chiude le IPCS e termina il server
*/
void sig_int_h(int sig)
{
	if (sig == SIGINT)
	{
		if (pid_client != -1)
			kill(pid_client, SIGUSR1);
		remove_ipcs();
		exit(0);
	}
}

int main(int argc, char *argv[])
{
	/*------------------------------------------------------------------------------------------------------
	VARIABILI LOCALI AL server
	------------------------------------------------------------------------------------------------------*/
	int times = 0;	  // utilizzato per prendere solo alla prima esecuzione il pid
					  // del client_0
	int number_files; // numero dei file letti
	struct messages_joined *file_divisi;
	pid_client = -1;

	/*--------------------------------------------------------------------------------------------------
   VARIABILI UTILIZZATE PER LA LETTURA DEI MESSAGGI SULLE IPCS
   ---------------------------------------------------------------------------------------------------*/
	size_t mSize = sizeof(struct message) - sizeof(long); // size del messaggio
	int k;												  // contatore di file completi
	char header[500];									  // buffer per salvare la stringa concatenata sui file _out
	char number[2];										  // utilizzato per la concatenazione
	char pid[10];										  // utilizzato per la concatenazione
	char ipcs[4][20] = {"FIFO1", "FIFO2", "MsgQueue",
						"ShdMem"}; // utilizzato per la concatenazione
	struct message *messaggio;	   // array di struct che contiene 4 parti di file
								   // lette da 4 IPC diverse
	int flag;					   // quando trovo un valore da leggere all'interno della shared memory
								   // lo setto ad 1
	int l;						   // indice dell'array di supporto per indicare le posizioni libere della
								   // shared memory
	// imposta l'handler al segnale SIGINT
	if (signal(SIGINT, sig_int_h) == SIG_ERR)
		err_exit("errore con l'handler di SIGINT");
	// funzione che crea le 4 IPCS necessarie per la comunicazione con il client
	create_ipcs();
	while (1)
	{
		k = 0;
		/*-------------------------------------------------------------------------------------------------
		LETTURA DELE INFORMAZIONI DA FIFO1
		--------------------------------------------------------------------------------------------------*/
		messaggio = (struct message *)calloc(4, sizeof(struct message));
		if (read(fifo1FD, &number_files, sizeof(int)) == -1)
			err_exit("read -");

		//riceviamo il numero dei files e alziamo il flag sulla shmem
		semOp(semID, 1, -1);
		(p1)->indici_messagi[0]=1;
		semOp(semID, 1, 1);
		semOp(semID, 0, 1);

		// lettura dalla FIFO1 del pid di client_0, avverrà solo una volta
		times++;
		if (times == 1)
		{
			if (read(fifo1FD, &pid_client, sizeof(pid_t)) == -1)
				err_exit("read -");
		}

		// contenitore dei messaggi, utilizzato per ricostruire il messaggio
		file_divisi = (struct messages_joined *)calloc(
			number_files, sizeof(struct messages_joined));
		// array di PID
		// ogni file è suddiviso in 4 parti da un processo ----> per riconoscere se
		// un file è di un processo, utilizziamo il pid
		int ids_messages[number_files];

		// Inizializzazione del contatore di file divisi degli ID a zero
		for (int i = 0; i < number_files; i++)
		{
			file_divisi[i].cont = 0;
			ids_messages[i] = 0;
		}

		// lettura fino a che non leggo tutti i send_me
		/*---------------------------------------------------------------------------------------------------
		LETTURA DALLE IPCS FINO A CHE NON LEGGO TUTTI I FILE
		----------------------------------------------------------------------------------------------------*/
		while (k < number_files)
		{
			/*-----------------------------------------------------------------------------------------------
			FIFO1
			------------------------------------------------------------------------------------------------*/
			if (read(fifo1FD, &messaggio[0], sizeof(struct message)) == -1)
				err_exit("read -");
			else
				semOp(semID, 2, 1);
			/*-----------------------------------------------------------------------------------------------
			FIFO2
			------------------------------------------------------------------------------------------------*/
			if (read(fifo2FD, &messaggio[1], sizeof(struct message)) == -1)
				err_exit("read -");
			else
				semOp(semID, 3, 1);
			/*-----------------------------------------------------------------------------------------------
			MSGQUEUE
			------------------------------------------------------------------------------------------------*/
			if (msgrcv(msgqID, &messaggio[2], mSize, 1, 0) == -1)
				err_exit("read -");
			else
				semOp(semID, 4, 1);

			/*-----------------------------------------------------------------------------------------------
			SHMEM
			------------------------------------------------------------------------------------------------*/
			flag = 0;
			l = 0;
			while (flag == 0)
			{
				if ((p1)->indici_messagi[l] == 1)
				{
					semOp(semID, 1, -1);
					messaggio[3] = (p1)->lista_messaggi[l];
					(p1)->indici_messagi[l] = 0;
					semOp(semID, 1, 1);
					flag = 1;
					semOp(semID, 5, 1);
				}
				l++;
				l %= 50;
			}
			/*-----------------------------------------------------------------------------------------------
			RICOMPOSIZIONE DEI 4 MESSAGGI
			------------------------------------------------------------------------------------------------*/
			for (int i = 0; i < 4; i++)
			{
				for (int j = 0; j < number_files; j++)
				{
					/*-----------------------------------------------------------------------------------
					INCREMENTO IL CONTATORE DI NUMERO DI FILE SE SONO ARRIVATI NUOVI FILE
					------------------------------------------------------------------------------------*/
					if (ids_messages[j] == 0)
					{
						file_divisi[j].lista_messaggi[i] = messaggio[i];
						ids_messages[j] = messaggio[i].pid;
						file_divisi[j].cont++;
						break;
					}
					/*-----------------------------------------------------------------------------------
					INCREMENTO IL CONTATORE DI NUMERO DI FILE SE SONO ARRIVATI NUOVI FILE
					------------------------------------------------------------------------------------*/
					else if (ids_messages[j] == messaggio[i].pid)
					{
						file_divisi[j].lista_messaggi[i] = messaggio[i];
						file_divisi[j].cont++;
						/*------------------------------------------------------------------------------
						SE SONO ARRIVATI 4 MESSAGGI DALLO STESSO PROCESSO, GENERO L' _out
						-------------------------------------------------------------------------------*/
						if (file_divisi[j].cont == 4)
						{
							struct message *mes =
								file_divisi[j]
									.lista_messaggi; // contiene i 4 messaggi del processo
							char *path = (char *)malloc(
								sizeof(mes[0].path_name) +
								10); // path del file da cui sono stati generati i messaggi
							strcpy(path, mes[0].path_name);
							path[strlen(mes[0].path_name) - 4] = '\0';
							strcat(path, "_out.txt");
							int fd =
								open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
							if (fd == -1)
								err_exit("open failed -");

							for (int j = 0; j < 4; j++)
							{
								strcpy(header, "[Parte ");
								sprintf(number, "%d", j + 1);
								strcat(header, number);
								strcat(header, " del file ");
								strcat(header, mes[j].path_name);
								strcat(header, ", spedita dal processo ");
								sprintf(pid, "%d", mes[j].pid);
								strcat(header, pid);
								strcat(header, " tramite ");
								strcat(header, ipcs[j]);
								strcat(header, "]\0");
								if (write(fd, header, strlen(header)) == -1)
									err_exit("write failed -");
								if (write(fd, "\n", 1) == -1)
									err_exit("write failed -");
								if (write(fd, mes[j].data, strlen(mes[j].data)) == -1)
									err_exit("write failed -");
								if (write(fd, "\n\n", 2) == -1)
									err_exit("write failed -");
								strcpy(header, "");
							}
							free(path);
							close(fd);
						}
						break;
					}
				}
			}
			k++;
		}
		// invio del messaggio vuoto al client_0 per procedere
		struct message *m = (struct message *)malloc(sizeof(struct message));
		if (msgsnd(msgqID, &m, mSize, 0) == -1)
			err_exit("msgsnd -");
		semOp(semID, 0, 1);
		free(m);
		free(file_divisi);
		free(messaggio);
	}
}
