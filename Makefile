CC = gcc

# CFLAGS  = -Werror
CFLAGS += -Wall
CFLAGS += -Wextra
CFLAGS += -Wshadow
CFLAGS += -g

LDFLAGS = -lz

EXEC = packet

SENDER_SRC = src/sender.c src/pkt.c src/network.c src/utils.c
RECEIVER_SRC = src/receiver.c src/pkt.c src/network.c src/utils.c src/fifo.c


all : clean sender receiver

sender :
	$(CC) $(CFLAGS) $(SENDER_SRC) -o sender $(LDFLAGS)

receiver :
	$(CC) $(CFLAGS) $(RECEIVER_SRC) -o receiver $(LDFLAGS)


.PHONY: clean gitlog

clean :
	rm -f sender
	rm -f receiver

archive : gitlog
	zip archive -r "src/" "tests/" "rapport.pdf" "gitlog.stat" -x sender receiver > zip.log

gitlog : 
	git log --stat > gitlog.stat
