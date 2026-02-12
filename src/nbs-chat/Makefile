CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c11 -D_POSIX_C_SOURCE=200809L -O2
LDFLAGS =

BIN_DIR = ../../bin

# Shared library objects (used by both binaries)
LIB_SRCS = chat_file.c base64.c lock.c
LIB_OBJS = $(LIB_SRCS:.c=.o)

# nbs-chat CLI
CLI_SRCS = main.c
CLI_OBJS = $(CLI_SRCS:.c=.o)
CLI_TARGET = nbs-chat

# nbs-chat-terminal
TERM_SRCS = terminal.c
TERM_OBJS = $(TERM_SRCS:.c=.o)
TERM_TARGET = nbs-chat-terminal

.PHONY: all clean install

all: $(CLI_TARGET) $(TERM_TARGET)

$(CLI_TARGET): $(CLI_OBJS) $(LIB_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TERM_TARGET): $(TERM_OBJS) $(LIB_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

install: all
	mkdir -p $(BIN_DIR)
	cp $(CLI_TARGET) $(BIN_DIR)/
	cp $(TERM_TARGET) $(BIN_DIR)/

# Dependencies
main.o: main.c chat_file.h
terminal.o: terminal.c chat_file.h
chat_file.o: chat_file.c chat_file.h base64.h lock.h
base64.o: base64.c base64.h
lock.o: lock.c lock.h

clean:
	rm -f $(LIB_OBJS) $(CLI_OBJS) $(TERM_OBJS) $(CLI_TARGET) $(TERM_TARGET)
