gcc -DFWSIM -std=c99 \
main.c \
../../firmware/main/physics.c \
../../firmware/main/simulate.c \
../../firmware/main/control.c \
../../firmware/main/bangbang.c \
../../firmware/main/primitives/primitive.h \
../../firmware/main/primitives/*.c \
-o sim -lm
#./sim 
#../../firmware/main/primitives/physics.c \
#./sim test.csv 3 1000 1000 0 0 0 0 
#python show_sim.py test.csv
