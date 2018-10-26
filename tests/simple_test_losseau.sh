#!/bin/bash

valgrind ./receiver ::1 1234 1> fichier.dat 2> log_receiver.log &
sleep 3
valgrind ./sender_losseau -f src/sender.c ::1 1234 2> log_sender.log
sleep 3
diff -s fichier.dat src/sender.c