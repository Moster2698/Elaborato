#include "defines.h"

pid_t pid_client;

struct messages_joined
{
    struct message lista_messaggi[4];
    int cont;
};

void create_ipcs();