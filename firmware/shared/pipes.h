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

#define PIPE_OUT_MASK ((1 << PIPE_DRIVE) | (1 << PIPE_KICK) | (1 << PIPE_FAULT_OUT) | (1 << PIPE_FIRMWARE_OUT))
#define PIPE_IN_MASK ((1 << PIPE_FEEDBACK) | (1 << PIPE_FIRMWARE_IN))

#define PIPE_STATE_TRANSPORT_MASK ((1 << PIPE_DRIVE) | (1 << PIPE_FEEDBACK))
#define PIPE_INTERRUPT_MASK ((1 << PIPE_KICK) | (1 << PIPE_FAULT_OUT) | (1 << PIPE_FIRMWARE_OUT) | (1 << PIPE_FIRMWARE_IN))

#endif

