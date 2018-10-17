CC = gcc

# CFLAGS  = -Werror
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Wshadow
CFLAGS += -g

LDFLAGS = -lz

EXEC = packet

SENDER_SRC = src/sender.c src/pkt.c src/network.c
RECEIVER_SRC = src/receiver.c src/pkt.c src/network.c


all : clean sender receiver

bitch: sender receiver

sender :
	$(CC) $(CFLAGS) $(SENDER_SRC) -o sender $(LDFLAGS)

receiver :
	$(CC) $(CFLAGS) $(RECEIVER_SRC) -o receiver $(LDFLAGS)


.PHONY: clean

clean :
	rm sender
	rm receiver