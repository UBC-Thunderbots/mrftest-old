#ifndef PIPES_H
#define PIPES_H

#include <stdint.h>

#define PIPE_DRIVE 0
#define PIPE_FEEDBACK 1
#define PIPE_KICK 2
#define PIPE_FAULT_OUT 3
#define PIPE_FAULT_IN 4
#define PIPE_FIRMWARE_OUT 5
#define PIPE_FIRMWARE_IN 6
#define PIPE_MAX 6

extern const uint8_t pipe_out_mask;
extern const uint8_t pipe_in_mask;

extern const uint8_t pipe_state_transport_mask;
extern const uint8_t pipe_interrupt_mask;
extern const uint8_t pipe_bulk_mask;

#endif

