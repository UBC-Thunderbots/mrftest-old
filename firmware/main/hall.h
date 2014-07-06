#ifndef HALL_H
#define HALL_H

#include <stdbool.h>
#include <stdint.h>

void hall_init(void);
void hall_tick(bool dribbler);
int16_t hall_speed(unsigned int index);

#endif

