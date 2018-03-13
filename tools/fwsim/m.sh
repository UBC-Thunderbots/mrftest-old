#!/usr/bin/env bash
gcc -DFWSIM -std=gnu99 -I../../firmware/main \
-I../../firmware/main/primitives \
main.c \
../../firmware/main/physics.c \
../../firmware/main/simulate.c \
../../firmware/main/control.c \
../../firmware/main/bangbang.c \
../../firmware/main/wheels.c \
../../firmware/main/dribbler.c \
../../firmware/main/primitives/primitive.c \
../../firmware/main/primitives/primitive.h \
../../firmware/main/primitives/move.c \
../../firmware/main/primitives/shoot.c \
../../firmware/main/primitives/spin.c \
../../firmware/main/primitives/pivot.c \
../../firmware/main/primitives/jcatch.c \
../../firmware/main/primitives/stop.c \
../../firmware/main/primitives/dribble.c \
../../firmware/main/util/physbot.c \
../../firmware/main/util/util.c \
../../firmware/main/util/log.c \
-o sim -lm\

# Python
X_FINAL=2000
Y_FINAL=1000
./sim test.csv 2 $X_FINAL $Y_FINAL 0 0 0 0
python3 show_sim.py test.csv



#../../firmware/main/primitives/direct_wheels.c \
#../../firmware/main/primitives/direct_velocity.c \
#
