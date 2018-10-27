#!/bin/bash

head -c 10000000 /dev/urandom > input.dat
# ./link_sim -p 1234 -P 12343 &
./receiver ::1 12343 1> fichier.dat 2> log_receiver.log &
sleep 0.5
./sender -f input.dat ::1 12343 2> log_sender.log 1>>output.log
# sleep 1
diff -s fichier.dat input.dat