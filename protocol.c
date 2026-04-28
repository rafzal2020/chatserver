#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "protocol.h"

/*
 * read_exactly - helper to read exactly n bytes from fd into buf.
 * Returns 0 on success, -1 if connection closed or error.
 */
static int read_exactly(int fd, char *buf, int n) {
    int total = 0;
    while (total < n) {
        int r = read(fd, buf + total, n - total);
        if (r <= 0) return -1;   // 0 = disconnected, <0 = error
        total += r;
    }
    return 0;
}

/*
 * read_until_bar - reads one byte at a time until '|', storing into buf.
 * buf will be null-terminated, NOT including the '|'.
 * max is the max number of characters before the '|' (not counting null).
 * Returns number of chars read (not counting '|'), or -1 on error/disconnect.
 */
static int read_until_bar(int fd, char *buf, int max) {
    int i = 0;
    while (1) {
        char c;
        int r = read(fd, &c, 1);
        if (r <= 0) return -1;
        if (c == '|') {
            buf[i] = '\0';
            return i;
        }
        if (i >= max) return -1;  // field too long — malformed
        buf[i++] = c;
    }
}

/*
 * read_message - reads one complete protocol message from fd.
 *
 * Format:  version|code|length|<body>
 * Example: 1|NAM|4|Bob|
 *
 * The body_len field tells us exactly how many bytes follow the third '|'.
 * We read exactly that many bytes — no more, no less.
 */
int read_message(int fd, char *code, char *body, int *body_len) {
    char version_buf[8];
    char length_buf[8];

    // Read version field (up to 7 chars before '|')
    if (read_until_bar(fd, version_buf, 7) < 0) return -1;

    // Spec says version is always "1"
    if (strcmp(version_buf, "1") != 0) return -1;

    // Read code field — exactly 3 chars before '|'
    if (read_until_bar(fd, code, 3) < 0) return -1;
    if (strlen(code) != 3) return -1;

    // Read length field (up to 5 digits before '|')
    if (read_until_bar(fd, length_buf, 5) < 0) return -1;

    // Convert length to integer
    int len = atoi(length_buf);
    if (len < 0 || len > MAX_BODY_LEN) return -1;

    // Read exactly len bytes for the body
    if (read_exactly(fd, body, len) < 0) return -1;
    body[len] = '\0';
    *body_len = len;

    // The body must end with '|' per the spec
    if (len == 0 || body[len - 1] != '|') return -1;

    return 0;
}

/*
 * send_message - formats and sends a complete protocol message.
 *
 * body must already end with '|'.
 * Example: send_message(fd, "MSG", "#all|Bob|Hello!|")
 */
void send_message(int fd, const char *code, const char *body) {
    int body_len = strlen(body);
    char header[32];
    // Format: "1|CODE|12345|"
    int header_len = snprintf(header, sizeof(header), "1|%s|%d|", code, body_len);
    // Write header then body as two separate writes
    write(fd, header, header_len);
    write(fd, body, body_len);
}

/*
 * send_error - sends an ERR message to the client.
 *
 * Format of body: "code|explanation|"
 * Example: "1|Smiling politely is in use|"
 */
void send_error(int fd, int err_code, const char *explanation) {
    char body[256];
    snprintf(body, sizeof(body), "%d|%s|", err_code, explanation);
    send_message(fd, CODE_ERR, body);
}

/*
 * parse_fields - splits body into fields on '|' boundaries.
 *
 * Modifies body in place by replacing '|' with '\0'.
 * Stops splitting after max_fields-1 splits so the last field
 * can contain embedded '|' characters (e.g. message text).
 *
 * Returns the number of fields found.
 */
int parse_fields(char *body, int body_len, char **fields, int max_fields) {
    int count = 0;
    int i = 0;

    while (i < body_len && count < max_fields) {
        // If this is the last available slot, take everything remaining
        // (minus trailing '|') so embedded '|' chars are preserved
        if (count == max_fields - 1) {
            fields[count++] = body + i;
            int end = body_len - 1;
            if (end >= 0 && body[end] == '|') body[end] = '\0';
            break;
        }

        // Normal field: record start, scan to next '|', null-terminate
        fields[count++] = body + i;
        while (i < body_len && body[i] != '|') i++;
        if (i < body_len) {
            body[i] = '\0';
            i++;
        }
    }
    return count;
}
