#!/bin/bash

./receiver ::1 1234 > fichier.dat 2> log_receiver.log &
sleep 3
./sender -f src/sender.c ::1 1234 > log_sender.log
sleep 3
diff -s fichier.dat src/sender.c
