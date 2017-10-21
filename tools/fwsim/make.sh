#!bin/bash
gcc -std=c99 *.c -o sim -lm
./sim > test.csv
python show_sim.py test.csv
#./sim