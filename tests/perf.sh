#!/bin/bash

# truncate -s 131073 input.dat
head -c 10000 /dev/urandom > input.dat
./receiver ::1 12344 1> fichier.dat 2> log_receiver.log &
sleep 1
./sender -f input.dat ::1 12344 2> log_sender.log
sleep 1
diff -s fichier.dat input.dat
