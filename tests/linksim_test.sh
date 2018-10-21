#!/bin/bash

./link_sim -p 2345 -P 12345 &
./receiver :: 12345 1> fichier.dat 2> tests/rec.log &
sleep 1
./sender -f src/sender.c ::1 2345 > tests/sen.log &
sleep 3
diff -s fichier.dat src/sender.c