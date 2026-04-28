#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "../client.h"
#include "../protocol.h"
#include "../handlers.h"

int pipe_alice[2];
int pipe_bob[2];

void setup_test_clients() {
    pipe(pipe_alice);
    pipe(pipe_bob);

    pthread_mutex_lock(&clients_lock);
    // Setup Alice in slot 0
    clients[0].active = 1;
    clients[0].fd = pipe_alice[1];
    strcpy(clients[0].name, "Alice");
    clients[0].status[0] = '\0';

    // Setup Bob in slot 1
    clients[1].active = 1;
    clients[1].fd = pipe_bob[1];
    strcpy(clients[1].name, "Bob");
    clients[1].status[0] = '\0';
    pthread_mutex_unlock(&clients_lock);
}

void teardown_test_clients() {
    close(pipe_alice[0]); close(pipe_alice[1]);
    close(pipe_bob[0]); close(pipe_bob[1]);
    pthread_mutex_lock(&clients_lock);
    client_remove(0);
    client_remove(1);
    pthread_mutex_unlock(&clients_lock);
}

void test_msg_to_all() {
    printf("TEST: Broadcast MSG to #all ... ");
    setup_test_clients();

    char *fields[] = {"", "#all", "Hello everyone!"};
    handle_msg(0, fields, 3); // Alice sends to #all

    char code[4], body[MAX_MSG_BODY];
    int body_len;

    // Bob should receive it
    read_message(pipe_bob[0], code, body, &body_len);
    if (strcmp(code, "MSG") == 0 && strstr(body, "Alice|#all|Hello everyone!") != NULL) {
        printf("PASS\n");
    } else {
        printf("FAIL\n");
    }
    teardown_test_clients();
}

void test_private_msg() {
    printf("TEST: Private MSG to Bob ... ");
    setup_test_clients();

    char *fields[] = {"", "Bob", "Secret message!"};
    handle_msg(0, fields, 3); // Alice sends to Bob

    char code[4], body[MAX_MSG_BODY];
    int body_len;

    // Bob should receive it
    read_message(pipe_bob[0], code, body, &body_len);
    if (strcmp(code, "MSG") == 0 && strstr(body, "Alice|Bob|Secret message!") != NULL) {
        printf("PASS\n");
    } else {
        printf("FAIL\n");
    }
    teardown_test_clients();
}

void test_unknown_recipient() {
    printf("TEST: MSG to unknown recipient ... ");
    setup_test_clients();

    char *fields[] = {"", "Charlie", "Are you there?"};
    handle_msg(0, fields, 3); // Alice sends to Charlie (doesn't exist)

    char code[4], body[MAX_MSG_BODY];
    int body_len;

    // Alice should receive ERR_UNKNOWN_RECIPIENT (code 2)
    read_message(pipe_alice[0], code, body, &body_len);
    if (strcmp(code, "ERR") == 0 && body[0] == '2') {
        printf("PASS\n");
    } else {
        printf("FAIL\n");
    }
    teardown_test_clients();
}

int main() {
    printf("=== handlers.c (MSG) tests ===\n");
    test_msg_to_all();
    test_private_msg();
    test_unknown_recipient();
    printf("==============================\n");
    return 0;
}
