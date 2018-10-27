#!/bin/bash

# truncate -s 131073 input.dat
head -c 131073 /dev/urandom > input.dat
valgrind ./receiver -f fichier.dat ::1 12344  2> log_receiver.log &
sleep 3
valgrind ./sender -f input.dat ::1 12344 2> log_sender.log
sleep 3
diff -s fichier.dat input.dat
