/// @file client_defines.h
/// @brief Contiene la definizioni di variabili
///         e funzioni specifiche del progetto.
#pragma once
#include "defines.h"
#include <sys/wait.h>

// struttura per la gestione dei file "send_me"
struct info_files
{
    // Quanti file sono conformi alle richieste
    int number_file;
    // array di file_descriptor
    int file_descriptors[100];
    // dimensione di ogni file
    size_t file_dimension[100];
    char paths[100][200];
};

// struttura per il contenuto dei messaggi nelle IPC's
void recursive_dir_read(char *directory_name);
struct info_files read_all_directories(char *directory_name);
void file_divider(struct info_files number_files);
void init_ipcs();
void write_on_ipcs(struct message messaggi[4]);