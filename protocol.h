#ifndef PROTOCOL_H
#define PROTOCOL_H

#define MAX_MSG_BODY 100000  // 5-digit length field max

// Message codes
#define CODE_NAM "NAM"
#define CODE_SET "SET"
#define CODE_MSG "MSG"
#define CODE_WHO "WHO"
#define CODE_ERR "ERR"

#define ERR_UNREADABLE        0
#define ERR_NAME_IN_USE       1
#define ERR_UNKNOWN_RECIPIENT 2
#define ERR_ILLEGAL_CHAR      3
#define ERR_TOO_LONG          4

int  read_message(int fd, char *code, char *body, int *body_len);
void send_message(int fd, const char *code, const char *fmt, ...);
void send_error(int fd, int code, const char *explanation);
int  parse_fields(char *body, int body_len, char **fields, int max_fields);

#endif

