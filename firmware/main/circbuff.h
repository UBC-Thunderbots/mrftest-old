#ifndef CIRCBUFF_H
#define CIRCBUFF_H

#include <stdbool.h>

typedef struct 
{
	float speed_x;
	float speed_y;
	float speed_angle;
} wheel_speeds_t;

//const int queueSize = 50;
//volatile unsigned char buffer[queueSize];

void circbuff_init(speed_t queue[], unsigned int queueSize);
void addToQueue(speed_t queue[], unsigned int queueSize, speed_t value);
speed_t getFromQueue(speed_t queue[], unsigned int queueSize, unsigned int index);

#endif // CIRCBUFF_H
