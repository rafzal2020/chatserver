#ifndef CLIENT_H
#define CLIENT_H

#include <pthread.h>

#define MAX_CLIENTS 128
#define MAX_NAME 32
#define MAX_STATUS 64

typedef struct {
    int fd;
    char name[MAX_NAME + 1];
    char status[MAX_STATUS + 1];
    int active;
} Client;

extern Client clients[MAX_CLIENTS];
extern pthread_mutex_t clients_lock;

int  client_find_slot();
void client_remove(int slot);
int  client_find_by_name(const char *name);

#endif
