#!/bin/bash

rm -f input.dat
head -c 131073 /dev/urandom > input.dat
valgrind ./receiver ::1 1234 1> fichier.dat 2> log_receiver.log &
sleep 3
valgrind ./sender_losseau -f input.dat ::1 1234 2> log_sender.log
sleep 3
diff -s fichier.dat input.dat