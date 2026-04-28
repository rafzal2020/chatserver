#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../client.h"
#include "../protocol.h"
#include "../handlers.h"

int pipe_fds[2];

// Helper to fake a connected client
void setup_test_client(int slot, const char *existing_name) {
    pipe(pipe_fds);
    pthread_mutex_lock(&clients_lock);
    clients[slot].fd = pipe_fds[1]; // Write end goes to the handler
    clients[slot].active = 1;
    if (existing_name) {
        strcpy(clients[slot].name, existing_name);
    } else {
        clients[slot].name[0] = '\0';
    }
    pthread_mutex_unlock(&clients_lock);
}

void teardown_test_client(int slot) {
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    pthread_mutex_lock(&clients_lock);
    client_remove(slot);
    pthread_mutex_unlock(&clients_lock);
}

void test_valid_nam() {
    printf("TEST: Valid NAM ... ");
    setup_test_client(0, NULL);

    char *fields[] = {"Alice"};
    handle_nam(0, fields, 1);

    char code[4], body[MAX_MSG_BODY];
    int body_len;
    read_message(pipe_fds[0], code, body, &body_len);

    if (strcmp(code, "MSG") == 0 && strstr(body, "Welcome") != NULL) {
        printf("PASS\n");
    } else {
        printf("FAIL (Got code: %s)\n", code);
    }
    teardown_test_client(0);
}

void test_invalid_char() {
    printf("TEST: Invalid character ... ");
    setup_test_client(0, NULL);

    char *fields[] = {"Bad Name!"}; // space and ! are illegal
    handle_nam(0, fields, 1);

    char code[4], body[MAX_MSG_BODY];
    int body_len;
    read_message(pipe_fds[0], code, body, &body_len);

    if (strcmp(code, "ERR") == 0 && body[0] == '3') { // ERR_ILLEGAL_CHAR is 3
        printf("PASS\n");
    } else {
        printf("FAIL\n");
    }
    teardown_test_client(0);
}

void test_duplicate_name() {
    printf("TEST: Duplicate name ... ");
    // Put Bob in slot 1
    pthread_mutex_lock(&clients_lock);
    clients[1].active = 1;
    strcpy(clients[1].name, "Bob");
    pthread_mutex_unlock(&clients_lock);

    // Try to register Bob in slot 0
    setup_test_client(0, NULL);
    char *fields[] = {"Bob"};
    handle_nam(0, fields, 1);

    char code[4], body[MAX_MSG_BODY];
    int body_len;
    read_message(pipe_fds[0], code, body, &body_len);

    if (strcmp(code, "ERR") == 0 && body[0] == '1') { // ERR_NAME_IN_USE is 1
        printf("PASS\n");
    } else {
        printf("FAIL\n");
    }
    
    teardown_test_client(0);
    pthread_mutex_lock(&clients_lock);
    client_remove(1);
    pthread_mutex_unlock(&clients_lock);
}

int main() {
    printf("=== handlers.c (NAM) tests ===\n");
    test_valid_nam();
    test_invalid_char();
    test_duplicate_name();
    printf("==============================\n");
    return 0;
}
