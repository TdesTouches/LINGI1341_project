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

test_losseau:
	tests/simple_test_losseau.sh

test_quinet:
	tests/simple_test_quinet.sh

big_tests:
	tests/big_test.sh

perf_test: all
	tests/perf.sh

tests :
	tests/simple_test.sh
	# tests/linksim_test.sh


.PHONY: clean gitlog tests

clean :
	rm -f sender
	rm -f receiver
	rm -f *.log
	rm -f *.dat
	rm -f *.tmp

archive : gitlog
	cp report/rapport.pdf rapport.pdf
	zip projet1_gennart -r "src/" "tests/" "rapport.pdf" "gitlog.stat" \
	 	"Makefile" -x sender receiver > zip.log

gitlog :
	git log --stat > gitlog.stat
