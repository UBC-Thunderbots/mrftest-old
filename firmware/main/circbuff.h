#ifndef CIRCBUFF_H
#define CIRCBUFF_H

typedef struct 
{
	float speed_x;
	float speed_y;
	float speed_t;
} speed;

//const int queueSize = 50;
//volatile unsigned char buffer[queueSize];
unsigned int start = 0; // Keeps track of the top most element in queue (the oldest element, the first one that will be read)
unsigned int stop = 0; // Keeps track of the latest element in the queue (the last one that will be read from all element currently in the queue)
bool queueEmpty = TRUE;

void addToQueue(speed* queue, unsigned int index, speed value);
speed* getFromQueue(speed* queue, unsigned int queueSize, unsigned int index);

#endif // CIRCBUFF_H