#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "protocol.h"


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
// In protocol.c
int read_message(int fd, char *code, char *body, int *body_len) {
    char c;
    int r;

    // Only eat whitespace (\n, \r, spaces)
    while (1) {
        r = read(fd, &c, 1);
        if (r <= 0) return -1; // Disconnected
        if (c == '\n' || c == '\r' || c == ' ') continue; 
        break; 
    }

    // Check Version (Fatal Error 0 if not '1')
    if (c != '1') {
        return -2; // Special code for "Unknown Version"
    }

    // Read the rest of the version header until the '|'
    r = read(fd, &c, 1);
    if (r <= 0 || c != '|') return -1; // Ill-formed

    // Read Code (3 chars)
    if (read_until_bar(fd, code, 3) < 0) return -1;

    // Read Length
    char len_str[16];
    if (read_until_bar(fd, len_str, 15) < 0) return -1;
    int length = atoi(len_str);
    *body_len = length;

    // Read Body
    r = read(fd, body, length);
    if (r != length) return -1; // Ill-formed (math mismatch)
    body[length] = '\0';

    // Final Bar Check (Fatal Error 0 if missing)
    if (body[length - 1] != '|') {
	    printf("DEBUG: Protocol Violation. Expected '|' at byte %d, got '%c'\n",
                length - 1, body[length - 1]);
	    return -1;
    }

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
    char header[64];
    // Format: "1|CODE|12345|"
    int header_len = snprintf(header, sizeof(header), "1|%s|%d|", code, body_len);
    // Write header then body as two separate writes
    write(fd, header, header_len);
    write(fd, body, body_len);

    write(fd, "\n", 1);
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
