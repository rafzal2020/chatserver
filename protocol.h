#ifndef PROTOCOL_H
#define PROTOCOL_H

// Message codes
#define CODE_NAM "NAM"
#define CODE_SET "SET"
#define CODE_MSG "MSG"
#define CODE_WHO "WHO"
#define CODE_ERR "ERR"

// Error codes (from Table 1 in spec)
#define ERR_UNREADABLE        0   // fatal: ill-formed, close connection
#define ERR_NAME_IN_USE       1   // recoverable: screen name already taken
#define ERR_UNKNOWN_RECIPIENT 2   // recoverable: no such user or room
#define ERR_ILLEGAL_CHAR      3   // recoverable: bad character
#define ERR_TOO_LONG          4   // recoverable: field too long

// Max sizes from spec
#define MAX_BODY_LEN  99999       // 5-digit length field max
#define MAX_MSG_BODY  4096        // working buffer size

int  read_message(int fd, char *code, char *body, int *body_len);
void send_message(int fd, const char *code, const char *body);
void send_error(int fd, int err_code, const char *explanation);
int  parse_fields(char *body, int body_len, char **fields, int max_fields);

#endif
