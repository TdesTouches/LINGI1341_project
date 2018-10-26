#!/bin/bash

valgrind ./receiver ::1 12344 1> fichier.dat 2> log_receiver.log &
sleep 3
valgrind ./sender -f src/sender.c ::1 12344 2> log_sender.log
sleep 3
diff -s fichier.dat src/sender.c
