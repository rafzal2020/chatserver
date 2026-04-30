# chatserver

Rayaan Afzal : ra965
Anthony Rahner : arr234

A simple chat server in C using a very basic protocol allowing client programs to connect, choose unique screen names, and send messages to each other or to the group.

## File Structure
* chatd.c - the main loop that runs the chat client given a specified port
* client.h / client.c - add/remove/lookup helpers for the list of clients
* protocol.h / protocol.c - protocols for reading, sending, and parsing messages
* handlers.h / handlers.c - handles screen names, messages, statuses, and who commands.
* tests/ - Directory containing our automated test suite for the server components.

## Test Plan & Strategy
We implemented an automated, modular testing strategy using Unix pipes to simulate client socket connections without needing to run the full networking loop. This allowed us to deterministically test edge cases and verify protocol constraints. We created dedicated test files for each core requirement:

1. **Protocol Parsing (test_protocol.c):** - Verified that `read_message` strictly requires the trailing '|'.
   - Ensured unsupported version numbers (e.g., "2|NAM...") immediately return an error.
   - Checked that `parse_fields` correctly preserves embedded '|' characters within the message text field.

2. **Name Registration (test_nam.c):**
   - Verified successful registration sends the correct Welcome message.
   - Tested that names with illegal characters (like spaces or '!') trigger `ERR_ILLEGAL_CHAR`.
   - Verified duplicate screen names trigger `ERR_NAME_IN_USE`.

3. **Status Updates (test_set.c):**
   - Verified that a valid status update correctly broadcasts to `#all`.
   - Verified that an empty status update updates the client struct silently without broadcasting.
   - Verified that characters outside the 32-126 range trigger `ERR_ILLEGAL_CHAR`.

4. **Message Routing (test_msg.c):**
   - Verified that messages routed to `#all` are broadcasted to all connected, named users.
   - Verified that private messages are routed exclusively to the target user's file descriptor.
   - Verified that sending a message to a non-existent user returns `ERR_UNKNOWN_RECIPIENT`.

5. **Who Queries (test_who.c):**
   - Tested that querying `#all` correctly formats a newline-separated list of all active users.
   - Verified that querying a specific user accurately returns either their status or "No status".
   - Verified that querying a non-existent user returns `ERR_UNKNOWN_RECIPIENT`.


