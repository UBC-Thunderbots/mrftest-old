gcc -DFWSIM -std=c99 \
*.c \
../../firmware/main/physics.c \
-o sim -lm
#./sim 
#../../firmware/main/primitives/physics.c \
