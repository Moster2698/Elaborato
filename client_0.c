/// @file client.c
/// @brief Contiene l'implementazione del client.

#include "client_defines.h"
#include "defines.h"
/*
handler che fa il detouch della shared memory e termina il processo alla
ricezione del segnale SIGUSR1 inviato dal server alla sua terminazione
*/



void sigHandler(int sig)
{
	char *home;
	if (sig == SIGUSR1)
	{
		if (p1 != (struct wrapper_message *)NULL)
			free_shared_memory(p1);
		errno = 0;
		// solo se il server non è stato creato
		if (p1 == (struct wrapper_message *)NULL)
		{
			if (semctl(semID, 0, IPC_RMID, NULL) == -1)
				if (errno != EIDRM)
					err_exit("no semctl-");
		}
		if ((home = getenv("HOME")) == (char *)NULL)
			err_exit("getenv failed -");
		char *fifo1_path = (char *)calloc(200, sizeof(char));
		char *fifo2_path = (char *)calloc(200, sizeof(char));
		strcat(strcpy(fifo1_path, home), "/FIFO1");
		strcat(strcpy(fifo2_path, home), "/FIFO2");
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

		exit(0);
	}
}

int main(int argc, char *argv[])
{
	/*
	------------------------------------------------------------------------------------------------------
	VARIABILI LOCALI AL client_0
	------------------------------------------------------------------------------------------------------
	*/
	sigset_t mySet; // set di segnali utiliazzati per la maschera
	char path[200];
	char cwd[150];
	char *home;
	struct info_files files;
	int i = 0;
	errno = 0;
	// controllo del numero di argomenti
	if (argc != 2)
	{
		printf("Argomenti non validi, uso corretto ./client_0 path");
		exit(0);
	}
	/*
	copia del path passato da linea di comando nella variabile ''path''
	*/
	strcpy(path, argv[1]);

	/*
	Imposta la maschera con tutti i segnali ad eccezione di SIGUSR1 e SIGINT
	*/
	sigfillset(&mySet);
	sigdelset(&mySet, SIGINT);
	sigdelset(&mySet, SIGUSR1);
	if (sigprocmask(SIG_SETMASK, &mySet, NULL) == -1)
		err_exit("errore nel set della maschera");

	/*
	modifichiamo gli handler di SIGUSR1 e SIGINT:
	  SIGUSR1 -> termina il processo
	  SIGINT -> riesegue la procedura di invio dei file
	*/
	if (signal(SIGUSR1, sigHandler) == SIG_ERR)
		err_exit("errore con l'handler di SIGURS1");
	if (signal(SIGINT, sigHandler) == SIG_ERR)
		err_exit("errore con l'handler di SIGINT");

	if (getcwd(cwd, 150) == (char *)NULL)
		err_exit("cwd failed - ");
	if ((home = getenv("HOME")) == (char *)NULL)
		err_exit("getenv failed -");
	char *fifo1_path = (char *)calloc(200, sizeof(char));
	strcat(strcpy(fifo1_path, home), "/FIFO1");
	char *fifo2_path = (char *)calloc(200, sizeof(char));
	strcat(strcpy(fifo2_path, home), "/FIFO2");
	// imposta la sua directory corrente ad un path passato da linea di comando
	// all’avvio del programma saluta l’utente stampando a video una stringa
	// sblocco i segnali
	while (1)
	{

		// Aspetto l'arrivo di un segnale SIGINT o SIGUR1
		pause();
		if (access(fifo1_path, F_OK) != 0)
		{
			printf("La fifo1 non esiste\n");
			exit(-1);
		}
		if (access(fifo2_path, F_OK) != 0)
		{
			printf("La fifo2 non esiste\n");
			exit(-1);
		}
		/*-------------------------------------------------------------------------------------
		ESECUZIONE DELL'ALGORITMO
		---------------------------------------------------------------------------------------*/
		// inizializzazione delle IPCS lato client solo la prima volta
		if (i == 0)
		{
			init_ipcs();
		}
		i++;
		// Aggiorno la maschera impostando tutte i segnali in essa
		sigfillset(&mySet);
		if (sigprocmask(SIG_SETMASK, &mySet, NULL) == -1)
			err_exit("errore nel set della maschera");

		// stampo la stringa richiesta
		printf(" ciao (%s) ora inizio la lettura dei file contenuti in %s\n",
			   getenv("USER"), path);
		if (chdir(path) == -1)
			err_exit("chdir -");
		// Salvataggio dei file con le informazioni necessarie per ricomporre i messaggi lato server
		files = read_all_directories(path);
		// invio il numero di file sulla FIFO1 convertendo l'intero in stringa
		if (write(fifo1FD, &files.number_file, sizeof(int)) == -1)
			err_exit("write on FIFO failed - ");
		

		// invio il pid di client_0 convertendo pid_t in stringa
		if (i == 1)
		{
			pid_t pid_client = getpid();
			if (write(fifo1FD, &pid_client, sizeof(pid_t)) == -1)
				err_exit("write pid -");
		}
		/*IPC: creazione del set di semafori:
		Il semaforo 0: gestisce la sincronizzazione con il server alla ricezione del numero di file
		Il semaforo 1: utilizzato per la mutua esclusione
		Il semaforo 2: utilizzato come contatore di messaggi nella FIFO1
		Il semaforo 3: utilizzato come contatore di messaggi nella FIFO2
		Il semaforo 4: utilizzato come contatore di messaggi nella MSGQUEUE
		Il semaforo 5: utilizzato come contatore di messaggi nella SHMEM
		*/
		semID = semget(KEY_SEM, 6, S_IWUSR | S_IRUSR);
		if (semID == -1)
			err_exit("semget failed - ");
		unsigned short semInitVal[] = {0, 1, 50, 50, 50, 50};
		union my_semun arg;
		arg.array = semInitVal;
		if (semctl(semID, 0, SETALL, arg) == -1)
			err_exit("semctl failed -");

		//aspettiamo la conferma del server su shMem;
		semOp(semID, 0, -1);
		semOp(semID, 1, -1);
		int check = (p1)->indici_messagi[0];
		if(check == 1)
		{
			printf("ho recuperato il messaggio\n");
			(p1)->indici_messagi[0]=0;
			semOp(semID, 1, 1);
		}
		else{
			printf("non ho recuperato il messaggio\n");
			semOp(semID, 1, 1);
			exit(-1);
		}
		
		// divisione dei file e invio sui quattro canali
		file_divider(files);

		// Aspetto l'arrivo di un messaggio vuoto da parte del server che indica il salvataggio dei file _out
		semOp(semID, 0, -1);
		struct message m;
		if (msgrcv(msgqID, &m, sizeof(struct message) - sizeof(long), 0, 0) == -1)
			err_exit("msgrcv - ");

		/*
		Imposta la maschera con tutti i segnali ad eccezione di SIGUSR1 e SIGINT
		*/
		sigfillset(&mySet);
		sigdelset(&mySet, SIGINT);
		sigdelset(&mySet, SIGUSR1);
		if (sigprocmask(SIG_SETMASK, &mySet, NULL) == -1)
			err_exit("errore nel set della maschera");
		if (semctl(semID, 0, SETALL, arg) == -1)
			err_exit("semctl failed -");

		if (chdir(cwd) == -1)
			err_exit("chdir -");
	}
}