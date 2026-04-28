#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "protocol.h"

// Helper: write raw bytes into the write-end of a pipe, then read_message from read-end
static int pipe_fds[2];

void setup_pipe() {
    if (pipe(pipe_fds) < 0) { perror("pipe"); exit(1); }
}

void write_raw(const char *s) {
    write(pipe_fds[1], s, strlen(s));
}

void close_write_end() {
    close(pipe_fds[1]);
}

// -----------------------------------------------------------------------

void test_read_nam() {
    printf("TEST read_message: NAM ... ");
    setup_pipe();
    write_raw("1|NAM|4|Bob|");
    close(pipe_fds[1]);

    char code[4];
    char body[MAX_MSG_BODY];
    int body_len;

    int r = read_message(pipe_fds[0], code, body, &body_len);
    close(pipe_fds[0]);

    if (r != 0)           { printf("FAIL: read_message returned %d\n", r); return; }
    if (strcmp(code, "NAM") != 0) { printf("FAIL: code='%s'\n", code); return; }
    if (body_len != 4)    { printf("FAIL: body_len=%d\n", body_len); return; }
    if (strcmp(body, "Bob|") != 0) { printf("FAIL: body='%s'\n", body); return; }
    printf("PASS\n");
}

void test_read_msg() {
    printf("TEST read_message: MSG ... ");
    setup_pipe();
    // From spec: 1|MSG|20||#all|Hello, world!|
    write_raw("1|MSG|20||#all|Hello, world!|");
    close(pipe_fds[1]);

    char code[4];
    char body[MAX_MSG_BODY];
    int body_len;

    int r = read_message(pipe_fds[0], code, body, &body_len);
    close(pipe_fds[0]);

    if (r != 0)           { printf("FAIL: read_message returned %d\n", r); return; }
    if (strcmp(code, "MSG") != 0) { printf("FAIL: code='%s'\n", code); return; }
    if (body_len != 20)   { printf("FAIL: body_len=%d\n", body_len); return; }
    printf("PASS\n");
}

void test_parse_fields_nam() {
    printf("TEST parse_fields: NAM body ... ");
    char body[] = "Bob|";
    char *fields[4];
    int n = parse_fields(body, strlen(body), fields, 4);

    if (n != 1)                    { printf("FAIL: n=%d\n", n); return; }
    if (strcmp(fields[0], "Bob") != 0) { printf("FAIL: fields[0]='%s'\n", fields[0]); return; }
    printf("PASS\n");
}

void test_parse_fields_msg() {
    printf("TEST parse_fields: MSG body (3 fields) ... ");
    // body for MSG from client: "|#all|Hello, world!|"
    // fields: sender(empty), recipient, message
    char body[] = "|#all|Hello, world!|";
    char *fields[4];
    int n = parse_fields(body, strlen(body), fields, 4);

    if (n != 3)  { printf("FAIL: n=%d\n", n); return; }
    if (strcmp(fields[0], "") != 0)             { printf("FAIL: fields[0]='%s'\n", fields[0]); return; }
    if (strcmp(fields[1], "#all") != 0)         { printf("FAIL: fields[1]='%s'\n", fields[1]); return; }
    if (strcmp(fields[2], "Hello, world!") != 0){ printf("FAIL: fields[2]='%s'\n", fields[2]); return; }
    printf("PASS\n");
}

void test_parse_fields_msg_with_pipe_in_text() {
    printf("TEST parse_fields: message body containing '|' ... ");
    // MSG body has 3 fields: sender, recipient, message_text
    // message text itself contains a pipe — last field should keep it
    // We pass max_fields=3 to say "stop splitting after 2 splits"
    char body[] = "|#all|Hello|world|";
    char *fields[3];
    int n = parse_fields(body, strlen(body), fields, 3);

    if (n != 3)  { printf("FAIL: n=%d\n", n); return; }
    if (strcmp(fields[0], "") != 0)                 { printf("FAIL: fields[0]='%s'\n", fields[0]); return; }
    if (strcmp(fields[1], "#all") != 0)             { printf("FAIL: fields[1]='%s'\n", fields[1]); return; }
    if (strcmp(fields[2], "Hello|world") != 0)      { printf("FAIL: fields[2]='%s'\n", fields[2]); return; }
    printf("PASS\n");
}

void test_send_and_read_roundtrip() {
    printf("TEST send_message + read_message roundtrip ... ");
    setup_pipe();

    // Write a MSG using send_message into the write end
    send_message(pipe_fds[1], "MSG", "#all|Bob|Hello, world!|");
    close(pipe_fds[1]);

    char code[4];
    char body[MAX_MSG_BODY];
    int body_len;

    int r = read_message(pipe_fds[0], code, body, &body_len);
    close(pipe_fds[0]);

    if (r != 0)           { printf("FAIL: read_message returned %d\n", r); return; }
    if (strcmp(code, "MSG") != 0) { printf("FAIL: code='%s'\n", code); return; }
    if (strcmp(body, "#all|Bob|Hello, world!|") != 0) { printf("FAIL: body='%s'\n", body); return; }
    printf("PASS\n");
}

void test_send_error() {
    printf("TEST send_error + read_message roundtrip ... ");
    setup_pipe();

    send_error(pipe_fds[1], ERR_NAME_IN_USE, "Name already taken");
    close(pipe_fds[1]);

    char code[4];
    char body[MAX_MSG_BODY];
    int body_len;

    int r = read_message(pipe_fds[0], code, body, &body_len);
    close(pipe_fds[0]);

    if (r != 0)           { printf("FAIL: read_message returned %d\n", r); return; }
    if (strcmp(code, "ERR") != 0) { printf("FAIL: code='%s'\n", code); return; }

    // Parse the ERR body: "1|Name already taken|"
    char *fields[4];
    int n = parse_fields(body, body_len, fields, 4);
    if (n != 2)  { printf("FAIL: n=%d\n", n); return; }
    if (strcmp(fields[0], "1") != 0)                  { printf("FAIL: err code='%s'\n", fields[0]); return; }
    if (strcmp(fields[1], "Name already taken") != 0)  { printf("FAIL: explanation='%s'\n", fields[1]); return; }
    printf("PASS\n");
}

void test_bad_version() {
    printf("TEST read_message: bad version returns -1 ... ");
    setup_pipe();
    write_raw("2|NAM|4|Bob|");  // version 2 is invalid
    close(pipe_fds[1]);

    char code[4];
    char body[MAX_MSG_BODY];
    int body_len;

    int r = read_message(pipe_fds[0], code, body, &body_len);
    close(pipe_fds[0]);

    if (r != -1) { printf("FAIL: expected -1, got %d\n", r); return; }
    printf("PASS\n");
}

void test_missing_trailing_bar() {
    printf("TEST read_message: body missing trailing '|' returns -1 ... ");
    setup_pipe();
    write_raw("1|NAM|3|Bob");   // length=3, body="Bob" with no trailing |
    close(pipe_fds[1]);

    char code[4];
    char body[MAX_MSG_BODY];
    int body_len;

    int r = read_message(pipe_fds[0], code, body, &body_len);
    close(pipe_fds[0]);

    if (r != -1) { printf("FAIL: expected -1, got %d\n", r); return; }
    printf("PASS\n");
}

int main() {
    printf("=== protocol.c tests ===\n");
    test_read_nam();
    test_read_msg();
    test_parse_fields_nam();
    test_parse_fields_msg();
    test_parse_fields_msg_with_pipe_in_text();
    test_send_and_read_roundtrip();
    test_send_error();
    test_bad_version();
    test_missing_trailing_bar();
    printf("========================\n");
    return 0;
}
