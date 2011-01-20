#ifndef SHARED_PIPES_H
#define SHARED_PIPES_H

#include <stdint.h>

#define PIPE_DRIVE 0
#define PIPE_FEEDBACK 1
#define PIPE_FAULT_OUT 2
#define PIPE_KICK 3
#define PIPE_FIRMWARE_OUT 4
#define PIPE_FIRMWARE_IN 5
#define PIPE_MAX 5

extern const uint8_t pipe_out_mask;
extern const uint8_t pipe_in_mask;

extern const uint8_t pipe_state_transport_mask;
extern const uint8_t pipe_interrupt_mask;

#endif

