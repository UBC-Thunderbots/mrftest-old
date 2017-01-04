#ifndef CIRCBUFF_H
#define CIRCBUFF_H

typedef struct 
{
	float speed_x;
	float speed_y;
	float speed_angle;
} speed_t;

//const int queueSize = 50;
//volatile unsigned char buffer[queueSize];
unsigned int start = 0; // Keeps track of the top most element in queue (the oldest element, the first one that will be read)
unsigned int stop = 0; // Keeps track of the latest element in the queue (the last one that will be read from all element currently in the queue)
bool queueEmpty = TRUE;

void circbuff_init(speed_t* queue, unsigned int queueSize);
void addToQueue(speed_t* queue, unsigned int index, speed_t value);
speed_t* getFromQueue(speed_t* queue, unsigned int queueSize, unsigned int index);

#endif // CIRCBUFF_H