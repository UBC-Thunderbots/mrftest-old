#ifndef RECEIVE_H
#define RECEIVE_H

#include <stdbool.h>

void receive_init(unsigned int index);
void receive_shutdown(void);
void receive_tick(void);
bool receive_drive_packet_timeout(void);

#endif

