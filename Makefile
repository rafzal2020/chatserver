CC = gcc
CFLAGS = -Wall -Wextra -g -I.
LDFLAGS = -lpthread

TARGET = chatd
SRCS = chatd.c client.c protocol.c handlers.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

# Test targets
test: test_protocol test_nam test_set test_msg test_who

test_protocol: tests/test_protocol.c protocol.c
	$(CC) $(CFLAGS) -o tests/test_protocol tests/test_protocol.c protocol.c
	./tests/test_protocol

test_nam: tests/test_nam.c handlers.c client.c protocol.c
	$(CC) $(CFLAGS) -o tests/test_nam tests/test_nam.c handlers.c client.c protocol.c $(LDFLAGS)
	./tests/test_nam

test_set: tests/test_set.c handlers.c client.c protocol.c
	$(CC) $(CFLAGS) -o tests/test_set tests/test_set.c handlers.c client.c protocol.c $(LDFLAGS)
	./tests/test_set

test_msg: tests/test_msg.c handlers.c client.c protocol.c
	$(CC) $(CFLAGS) -o tests/test_msg tests/test_msg.c handlers.c client.c protocol.c $(LDFLAGS)
	./tests/test_msg

test_who: tests/test_who.c handlers.c client.c protocol.c
	$(CC) $(CFLAGS) -o tests/test_who tests/test_who.c handlers.c client.c protocol.c $(LDFLAGS)
	./tests/test_who

clean:
	rm -f $(OBJS) $(TARGET) tests/test_protocol
