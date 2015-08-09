#ifndef RECEIVE_H
#define RECEIVE_H

#include <stdbool.h>
#include <stdint.h>

void receive_init(unsigned int index);
void receive_shutdown(void);
void receive_tick(void);
uint8_t receive_last_serial(void);

#endif

