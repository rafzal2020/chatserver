#include "client.h"
#include <string.h>

Client clients[MAX_CLIENTS];
pthread_mutex_t clients_lock = PTHREAD_MUTEX_INITIALIZER;

// Find a free slot — call with lock held
int client_find_slot() {
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (!clients[i].active) return i;
    return -1;
}

void client_remove(int slot) {
	clients[slot].active = 0;
	clients[slot].name[0] = '\0';
	clients[slot].status[0] = '\0';
}

int client_find_by_name(const char *name) {
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (clients[i].active && strcmp(clients[i].name, name) == 0)
            return i;
    return -1;
}
