#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "client.h"
#include "protocol.h"
#include "handlers.h"

void handle_nam(int slot, char **fields, int nfields) {
    pthread_mutex_lock(&clients_lock);
    int fd = clients[slot].fd;
    pthread_mutex_unlock(&clients_lock);

    if (nfields < 1) {
        send_error(fd, ERR_UNREADABLE, "Missing name field");
        return;
    }

    char *name = fields[0];
    int len = strlen(name);

    if (len < 1 || len > MAX_NAME) {
        send_error(fd, ERR_TOO_LONG, "Name must be 1-32 characters");
        return;
    }

    for (int i = 0; i < len; i++) {
    	// Use unsigned char to safely handle values above 127
    	unsigned char c = (unsigned char)name[i];

    	if (c < 32 || c > 126) {
        	send_error(fd, ERR_ILLEGAL_CHAR, "Invalid character detected");
        	return; // Recoverable: do NOT break the main loop in chatd.c
    	}
    }

    // Uniqueness check
    pthread_mutex_lock(&clients_lock);
    if (client_find_by_name(name) != -1) {
        pthread_mutex_unlock(&clients_lock);
        send_error(fd, ERR_NAME_IN_USE, "Name already taken");
        return;
    }

    // Set the name
    strcpy(clients[slot].name, name);

    // 1. Send Welcome to the NEW user
    char welcome_body[MAX_MSG_BODY];
    snprintf(welcome_body, sizeof(welcome_body), "#all|%s|Welcome to the chat!|", name);
    send_message(fd, CODE_MSG, welcome_body);

    // 2. Broadcast "Joined" to EVERYONE ELSE
    char join_msg[MAX_MSG_BODY];
    snprintf(join_msg, sizeof(join_msg), "#all|#all|%s has joined the chat|", name);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        // Only send to active users who have names and AREN'T the new person
        if (clients[i].active && strlen(clients[i].name) > 0 && i != slot) {
            send_message(clients[i].fd, CODE_MSG, join_msg);
        }
    }

    pthread_mutex_unlock(&clients_lock);

    printf("Slot %d registered name: %s\n", slot, name);
}

void handle_set(int slot, char **fields, int nfields) {
    pthread_mutex_lock(&clients_lock);
    int fd = clients[slot].fd;
    char name[MAX_NAME + 1];
    strcpy(name, clients[slot].name);
    pthread_mutex_unlock(&clients_lock);

    // Rule: User must have a name before sending messages 
    if (strlen(name) == 0) {
        send_error(fd, ERR_UNREADABLE, "Must set name first");
        return;
    }

    if (nfields < 1) {
        send_error(fd, ERR_UNREADABLE, "Missing status field");
        return;
    }

    char *status = fields[0];
    int len = strlen(status);

    // Rule: Status must be 0-64 characters 
    if (len > MAX_STATUS) {
        send_error(fd, ERR_TOO_LONG, "Status too long");
        return;
    }

    // Rule: Status characters must be in the range 32-126 
    for (int i = 0; i < len; i++) {
        unsigned char c = status[i];
        if (c < 32 || c > 126) {
            send_error(fd, ERR_ILLEGAL_CHAR, "Invalid character in status");
            return;
        }
    }

    // Update the status safely
    pthread_mutex_lock(&clients_lock);
    strcpy(clients[slot].status, status);

    // If status is non-empty, broadcast to all named users [cite: 59]
    if (len > 0) {
    	char message[256]; // <-- Changed this to 256 to fix the warning
    	snprintf(message, sizeof(message), "%s is now \"%s\"", name, status);
        
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active && strlen(clients[i].name) > 0) {
                char body[MAX_MSG_BODY];
                // Broadcast format: sender(#all)|recipient(#all)|message
                snprintf(body, sizeof(body), "#all|#all|%s|", message);
                send_message(clients[i].fd, CODE_MSG, body);
            }
        }
    }


    pthread_mutex_unlock(&clients_lock);

    printf("Slot %d (%s) updated status to: %s\n", slot, name, status);
} 

void handle_msg(int slot, char **fields, int nfields) {
    pthread_mutex_lock(&clients_lock);
    int fd = clients[slot].fd;
    char sender_name[MAX_NAME + 1];
    strcpy(sender_name, clients[slot].name);
    pthread_mutex_unlock(&clients_lock);

    // Rule: User must have a name before sending messages
    if (strlen(sender_name) == 0) {
        send_error(fd, ERR_UNREADABLE, "Must set name first");
        return;
    }

    if (nfields < 3) {
        send_error(fd, ERR_UNREADABLE, "Missing fields in MSG");
        return;
    }

    // fields[0] is sender (ignored from client), fields[1] is recipient, fields[2] is message
    char *recipient = fields[1];
    char *message = fields[2];
    int msg_len = strlen(message);

    // Rule: Message length 1-80
    if (msg_len < 1 || msg_len > 80) {
        send_error(fd, ERR_TOO_LONG, "Message must be 1-80 characters");
        return;
    }

    // Rule: Characters 32-126
    for (int i = 0; i < msg_len; i++) {
        unsigned char c = message[i];
        if (c < 32 || c > 126) {
            send_error(fd, 3, "Invalid character in message");
            return;
        }
    }

    // Format the outgoing message body: sender_name|recipient|message|
    char out_body[MAX_MSG_BODY];
    snprintf(out_body, sizeof(out_body), "%s|%s|%s|", sender_name, recipient, message);

    if (strcmp(recipient, "#all") == 0) {
        // Broadcast to all registered users
        pthread_mutex_lock(&clients_lock);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active && strlen(clients[i].name) > 0) {
                send_message(clients[i].fd, CODE_MSG, out_body);
            }
        }
        pthread_mutex_unlock(&clients_lock);
        printf("Slot %d (%s) sent MSG to #all\n", slot, sender_name);
    } else {
        // Private message
        pthread_mutex_lock(&clients_lock);
        int target_slot = client_find_by_name(recipient);
        
        if (target_slot == -1) {
            pthread_mutex_unlock(&clients_lock);
            send_error(fd, ERR_UNKNOWN_RECIPIENT, "Unknown recipient");
            return;
        }
        
        int target_fd = clients[target_slot].fd;
        pthread_mutex_unlock(&clients_lock);

        send_message(target_fd, CODE_MSG, out_body);
        printf("Slot %d (%s) sent private MSG to %s\n", slot, sender_name, recipient);
    }
}

void handle_who(int slot, char **fields, int nfields) {
    pthread_mutex_lock(&clients_lock);
    int fd = clients[slot].fd;
    char requester_name[MAX_NAME + 1];
    strcpy(requester_name, clients[slot].name);
    pthread_mutex_unlock(&clients_lock);

    // Rule: User must have a name before making requests
    if (strlen(requester_name) == 0) {
        send_error(fd, ERR_UNREADABLE, "Must set name first");
        return;
    }

    if (nfields < 1) {
        send_error(fd, ERR_UNREADABLE, "Missing target field");
        return;
    }

    char *target = fields[0];
    if (strcmp(target, "#all") == 0) {
        char payload[4000] = ""; // <-- Reduced to 4000 to leave room for headers
        int first = 1;

        // Loop through all active users to build the newline-separated list
        pthread_mutex_lock(&clients_lock);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active && strlen(clients[i].name) > 0) {
                char entry[256];
                if (strlen(clients[i].status) > 0) {
                    snprintf(entry, sizeof(entry), "%s: %s", clients[i].name, clients[i].status);
                } else {
                    snprintf(entry, sizeof(entry), "%s", clients[i].name);
                }

                if (!first) {
                    strcat(payload, "\n");
                }
                
                // Append this user's entry to the overall payload
                if (strlen(payload) + strlen(entry) + 2 < 4000) { // <-- Updated limit here
                    strcat(payload, entry);
                    first = 0;
                }
            }
        }
        pthread_mutex_unlock(&clients_lock);

        char out_body[MAX_MSG_BODY];
        snprintf(out_body, sizeof(out_body), "#all|%s|%s|", requester_name, payload);
        send_message(fd, CODE_MSG, out_body);
        printf("Slot %d (%s) requested WHO for #all\n", slot, requester_name);

    } else {
       // Query for a specific user
        pthread_mutex_lock(&clients_lock);
        int target_slot = client_find_by_name(target);
        
        if (target_slot == -1) {
            pthread_mutex_unlock(&clients_lock);
            send_error(fd, ERR_UNKNOWN_RECIPIENT, "Unknown user");
            return;
        }

        char target_status[MAX_STATUS + 1];
        strcpy(target_status, clients[target_slot].status);
        pthread_mutex_unlock(&clients_lock);

        char payload[256];
        if (strlen(target_status) > 0) {
            snprintf(payload, sizeof(payload), "%s: %s", target, target_status);
        } else {
            snprintf(payload, sizeof(payload), "No status");
        }

        char out_body[MAX_MSG_BODY];
        snprintf(out_body, sizeof(out_body), "#all|%s|%s|", requester_name, payload);
        send_message(fd, CODE_MSG, out_body);
        printf("Slot %d (%s) requested WHO for %s\n", slot, requester_name, target);
    }
}
