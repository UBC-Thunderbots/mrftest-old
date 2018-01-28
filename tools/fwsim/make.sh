# COMPILE
#!bin/bash
if gcc  -DFWSIM -std=c99 -Wall \
main.c \
../../firmware/main/physics.c \
../../firmware/main/simulate.c \
../../firmware/main/control.c \
../../firmware/main/primitives/primitive.h \
../../firmware/main/bangbang.c \
../../firmware/main/primitives/move.c \
../../firmware/main/primitives/shoot.c \
../../firmware/main/primitives/spin.c \
-o sim -lm; \
then
    if [ $? -ne 0 ]
    then
        echo "Compile failed!"
        exit 1
    fi
else
    echo "compile failed!"
    exit 1
fi

# PYTHON
./sim test.csv 3 1000 1000 0 0 0 0 
python show_sim.py test.csv

# FWVIEW
cp test.csv fwview/data/
bash fwview/fwView
