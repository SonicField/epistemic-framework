CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L -O2
LDFLAGS =

SRCS = main.c chat_file.c base64.c lock.c
OBJS = $(SRCS:.c=.o)
TARGET = nbs-chat-c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# Dependencies
main.o: main.c chat_file.h
chat_file.o: chat_file.c chat_file.h base64.h lock.h
base64.o: base64.c base64.h
lock.o: lock.c lock.h

clean:
	rm -f $(OBJS) $(TARGET)
