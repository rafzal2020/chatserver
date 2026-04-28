#ifndef HANDLERS_H
#define HANDLERS_H

void handle_nam(int slot, char **fields, int nfields);
void handle_set(int slot, char **fields, int nfields);
void handle_msg(int slot, char **fields, int nfields);
void handle_who(int slot, char **fields, int nfields);

#endif
