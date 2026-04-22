# chatserver

Rayaan Afzal : ra965

Anthony Rahner : arr234


A simple chat server in C using a very basic protocol allowing client programs to connect, choose unique screen names, and send messages to each other or to the group.

# file structure

chatd.c - the main loop that runs the chat client given a specified port (e.g. ./chatd 8080)

client.h - contains the Client struct, including character limits, etc.

client.c - add/remove/lookup helpers for the list of clients

protocol.h - message constants and functions declarations for protocols

protocol.c - contains protocols for reading, sending and parsing messages

handlers.c - handles screen names, messages, statuses, and who commands.
