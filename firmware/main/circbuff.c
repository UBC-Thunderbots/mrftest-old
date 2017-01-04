#include "circbuff.h"

void circbuff_init(speed_t* queue, unsigned int queueSize)
{
	speed_t zero;
	zero.speed_x = 0.0;
	zero.speed_y = 0.0;
	zero.speed_angle = 0.0;

	for(int i=0; i < queueSize; i++)
	{
		queue[i] = zero;
	}
}

void addToQueue(speed_t* queue, unsigned int queueSize, speed_t value)
{
	// If the stop pointer has made it all the way to the start pointer, we have added elements equal to the buffer size but haven't read anything
	// So the start element is going to be overwritten and so we should move its pointer ahead by one
	if(queueEmpty)
		queue[stop] = value;
	else
	{
		stop++;
		if (stop == queueSize)
		{
			stop = 0;
		}

		queue[stop] = value;

		/*if(stop == start)
		{
			start++;
			if(start == queueSize)
				start = 0;
		}*/
	}

	queueEmpty = FALSE;

}

speed_t* getFromQueue(speed_t* queue, unsigned int queueSize, unsigned int index)
{
	// Check if queue is empty or not. If it is return -1 instead
	//if(!queueEmpty)
	//{
		
	// Get array index of required element
	int idx = stop - index;
	while(idx < 0)
	{
		idx += queueSize;
	}

		// Return element at start pointer
	speed_t returnVal = buffer[idx];

		// If the start and stop pointer are at the same position, means we have read the last value in the queue.
		/*if (start == stop)
			queueEmpty = TRUE;
		else
			start++;

		if (start == queueSize)
			start = 0;*/
	return &returnVal;
	/*else
	{
		speed returnVal;
		returnVal.speed_x = 0;
		returnVal.speed_y = 0;
		returnVal.speed_t = 0;
		return &returnVal;
	}*/
}