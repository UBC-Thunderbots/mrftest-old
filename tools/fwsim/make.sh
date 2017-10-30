#!bin/bash
if gcc -std=c99 -Wall *.c -o sim -lm; then
	if [ $? -ne 0 ]
	then
		echo "Compile failed!"
		exit 1
	fi
else
	echo "compile failed!"
	exit 1
fi

./sim > test.csv
python show_sim.py test.csv
#./sim

# cp test.csv fwview/data/
# bash fwview/fwView
