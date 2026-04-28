#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include "client.h"
#include "protocol.h"
#include "handlers.h"

// --- Forward declaration ---
void *handle_client(void *arg);

// --- Main ---
int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);

    // 1. Create socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(1);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // 2. Bind
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        exit(1);
    }

    // 3. Listen
    if (listen(server_fd, 10) < 0) {
        perror("listen");
        exit(1);
    }

    printf("chatd listening on port %d\n", port);

    // 4. Accept loop
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            perror("accept");
            continue;
        }

        // Find a free slot in the clients array
        pthread_mutex_lock(&clients_lock);
        int slot = client_find_slot();

        if (slot == -1) {
            pthread_mutex_unlock(&clients_lock);
            close(client_fd);
            fprintf(stderr, "Server full, rejected a connection\n");
            continue;
        }

        // Initialize the slot
        clients[slot].fd = client_fd;
        clients[slot].active = 1;
        clients[slot].name[0] = '\0';
        clients[slot].status[0] = '\0';
        pthread_mutex_unlock(&clients_lock);

        // Spawn a thread for this client, passing the slot index
        int *slot_ptr = malloc(sizeof(int));
        *slot_ptr = slot;

        pthread_t thread;
        pthread_create(&thread, NULL, handle_client, slot_ptr);
        pthread_detach(thread);
    }

    close(server_fd);
    return 0;
}

// --- Client thread ---
void *handle_client(void *arg) {
    int slot = *(int *)arg;
    free(arg);

    // Get a local copy of the file descriptor so we don't need to lock the array constantly
    pthread_mutex_lock(&clients_lock);
    int fd = clients[slot].fd;
    pthread_mutex_unlock(&clients_lock);

    char code[4];
    char body[MAX_MSG_BODY];
    int body_len;

    // Message parsing loop
    // In chatd.c inside handle_client
    while (1) {
    	int res = read_message(fd, code, body, &body_len);

    	if (res == -1 || res == -2) {
        	// Error 0: Fatal (Ill-formed or Unknown Version)
        	send_error(fd, 0, "Unreadable or unknown version");
        	usleep(100000); // gives the client a time window to print the fatal error before closing out.
		break;
    	} else if (res < 0) {
        	// Normal client disconnect
        	break;
    	}

	char *fields[8];

    	// Parse fields
    	int nfields = parse_fields(body, body_len, fields, 8);

    	// Route to handlers
    	if (strcmp(code, "NAM") == 0) {
        	handle_nam(slot, fields, nfields);
    	} else if (strcmp(code, "SET") == 0) {
        	handle_set(slot, fields, nfields);
    	} else if (strcmp(code, "MSG") == 0) {
        	handle_msg(slot, fields, nfields);
    	} else if (strcmp(code, "WHO") == 0) {
        	handle_who(slot, fields, nfields);
    	} else {
        	// Error 0: Fatal (Unknown message type)
        	send_error(fd, 0, "Unknown message type");
        	break;
    	}
    }


    // Cleanup when client disconnects
    pthread_mutex_lock(&clients_lock);
    close(clients[slot].fd);
    client_remove(slot);
    printf("Client in slot %d disconnected.\n", slot);
    pthread_mutex_unlock(&clients_lock);

    return NULL;
}
