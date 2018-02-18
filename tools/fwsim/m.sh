gcc -DFWSIM -std=c99 \
main.c \
../../firmware/main/physics.c \
../../firmware/main/simulate.c \
../../firmware/main/control.c \
../../firmware/main/bangbang.c \
../../firmware/main/primitives/primitive.c \
../../firmware/main/primitives/move.c \
../../firmware/main/primitives/shoot.c \
../../firmware/main/primitives/spin.c \
-o sim -lm
#./sim 
#./sim test.csv 3 1000 1000 0 0 0 0 
#python show_sim.py test.csv

#../../firmware/main/primitives/pivot.c \
#../../firmware/main/primitives/jcatch.c \
#../../firmware/main/primitives/direct_wheels.c \
#../../firmware/main/primitives/direct_velocity.c \
#../../firmware/main/primitives/stop.c \
