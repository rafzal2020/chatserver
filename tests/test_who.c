#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../client.h"
#include "../protocol.h"
#include "../handlers.h"

int pipe_alice[2];

void setup_test_environment() {
    pipe(pipe_alice);

    pthread_mutex_lock(&clients_lock);
    // Alice (Requester)
    clients[0].active = 1;
    clients[0].fd = pipe_alice[1];
    strcpy(clients[0].name, "Alice");
    clients[0].status[0] = '\0';

    // Bob (Has Status)
    clients[1].active = 1;
    clients[1].fd = -1; // We don't need Bob's fd, just his data
    strcpy(clients[1].name, "Bob");
    strcpy(clients[1].status, "Smiling politely");

    // Carol (No Status)
    clients[2].active = 1;
    clients[2].fd = -1;
    strcpy(clients[2].name, "Carol");
    clients[2].status[0] = '\0';
    pthread_mutex_unlock(&clients_lock);
}

void teardown_test_environment() {
    close(pipe_alice[0]);
    close(pipe_alice[1]);
    pthread_mutex_lock(&clients_lock);
    client_remove(0);
    client_remove(1);
    client_remove(2);
    pthread_mutex_unlock(&clients_lock);
}

void test_who_specific_with_status() {
    printf("TEST: WHO for specific user with status ... ");
    setup_test_environment();

    char *fields[] = {"Bob"};
    handle_who(0, fields, 1);

    char code[4], body[MAX_MSG_BODY];
    int body_len;
    read_message(pipe_alice[0], code, body, &body_len);

    if (strcmp(code, "MSG") == 0 && strstr(body, "Bob: Smiling politely") != NULL) {
        printf("PASS\n");
    } else {
        printf("FAIL\n");
    }
    teardown_test_environment();
}

void test_who_specific_no_status() {
    printf("TEST: WHO for specific user without status ... ");
    setup_test_environment();

    char *fields[] = {"Carol"};
    handle_who(0, fields, 1);

    char code[4], body[MAX_MSG_BODY];
    int body_len;
    read_message(pipe_alice[0], code, body, &body_len);

    if (strcmp(code, "MSG") == 0 && strstr(body, "No status") != NULL) {
        printf("PASS\n");
    } else {
        printf("FAIL\n");
    }
    teardown_test_environment();
}

void test_who_all() {
    printf("TEST: WHO for #all ... ");
    setup_test_environment();

    char *fields[] = {"#all"};
    handle_who(0, fields, 1);

    char code[4], body[MAX_MSG_BODY];
    int body_len;
    read_message(pipe_alice[0], code, body, &body_len);

    // Should contain everyone and newlines
    if (strcmp(code, "MSG") == 0 && 
        strstr(body, "Alice") != NULL && 
        strstr(body, "Bob: Smiling politely") != NULL && 
        strstr(body, "Carol") != NULL &&
        strchr(body, '\n') != NULL) {
        printf("PASS\n");
    } else {
        printf("FAIL\n");
    }
    teardown_test_environment();
}

void test_who_unknown() {
    printf("TEST: WHO for unknown user ... ");
    setup_test_environment();

    char *fields[] = {"Dave"};
    handle_who(0, fields, 1);

    char code[4], body[MAX_MSG_BODY];
    int body_len;
    read_message(pipe_alice[0], code, body, &body_len);

    if (strcmp(code, "ERR") == 0 && body[0] == '2') {
        printf("PASS\n");
    } else {
        printf("FAIL\n");
    }
    teardown_test_environment();
}

int main() {
    printf("=== handlers.c (WHO) tests ===\n");
    test_who_specific_with_status();
    test_who_specific_no_status();
    test_who_all();
    test_who_unknown();
    printf("==============================\n");
    return 0;
}
