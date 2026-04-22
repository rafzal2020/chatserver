CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lpthread

TARGET = chatd
SRCS = chatd.c client.c protocol.c handlers.c
OBJS = $(SRCS:.c=.o)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) $(TARGET)
