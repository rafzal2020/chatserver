#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "../client.h"
#include "../protocol.h"
#include "../handlers.h"

int pipe_fds[2];

void setup_test_client(int slot, const char *existing_name) {
    pipe(pipe_fds);
    pthread_mutex_lock(&clients_lock);
    clients[slot].fd = pipe_fds[1];
    clients[slot].active = 1;
    if (existing_name) {
        strcpy(clients[slot].name, existing_name);
    } else {
        clients[slot].name[0] = '\0';
    }
    // Initialize status to empty
    clients[slot].status[0] = '\0'; 
    pthread_mutex_unlock(&clients_lock);
}

void teardown_test_client(int slot) {
    close(pipe_fds[0]);
    close(pipe_fds[1]);
    pthread_mutex_lock(&clients_lock);
    client_remove(slot);
    pthread_mutex_unlock(&clients_lock);
}

void test_valid_set_broadcasts() {
    printf("TEST: Valid SET broadcasts to #all ... ");
    setup_test_client(0, "Bob");

    char *fields[] = {"Smiling politely"};
    handle_set(0, fields, 1);

    char code[4], body[MAX_MSG_BODY];
    int body_len;
    read_message(pipe_fds[0], code, body, &body_len);

    // Should receive a MSG from #all to #all indicating the change
    if (strcmp(code, "MSG") == 0 && strstr(body, "Smiling politely") != NULL) {
        printf("PASS\n");
    } else {
        printf("FAIL (Got code: %s, body: %s)\n", code, body);
    }
    teardown_test_client(0);
}

void test_empty_set_silent() {
    printf("TEST: Empty SET does not broadcast ... ");
    setup_test_client(0, "Bob");

    char *fields[] = {""};
    handle_set(0, fields, 1);

    // Make the read non-blocking to check if nothing was sent
    int flags = fcntl(pipe_fds[0], F_GETFL, 0);
    fcntl(pipe_fds[0], F_SETFL, flags | O_NONBLOCK);

    char code[4], body[MAX_MSG_BODY];
    int body_len;
    int r = read_message(pipe_fds[0], code, body, &body_len);

    // r < 0 means no message was ready to be read, which is correct for an empty status
    if (r < 0) {
        printf("PASS\n");
    } else {
        printf("FAIL (Server sent a message when it shouldn't have)\n");
    }
    teardown_test_client(0);
}

void test_invalid_char_set() {
    printf("TEST: SET with invalid character ... ");
    setup_test_client(0, "Bob");

    // ASCII 31 is not allowed (range is 32-126)
    char bad_status[] = "Bad\x1FStatus";
    char *fields[] = {bad_status};
    handle_set(0, fields, 1);

    // Revert pipe to blocking
    int flags = fcntl(pipe_fds[0], F_GETFL, 0);
    fcntl(pipe_fds[0], F_SETFL, flags & ~O_NONBLOCK);

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

int main() {
    printf("=== handlers.c (SET) tests ===\n");
    test_valid_set_broadcasts();
    test_empty_set_silent();
    test_invalid_char_set();
    printf("==============================\n");
    return 0;
}
