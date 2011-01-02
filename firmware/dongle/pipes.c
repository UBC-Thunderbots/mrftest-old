#include "pipes.h"

const uint8_t pipe_out_mask = (1 << PIPE_DRIVE) | (1 << PIPE_KICK) | (1 << PIPE_FAULT_OUT) | (1 << PIPE_FIRMWARE_OUT);
const uint8_t pipe_in_mask = (1 << PIPE_FEEDBACK) | (1 << PIPE_FAULT_IN) | (1 << PIPE_FIRMWARE_IN);

const uint8_t pipe_state_transport_mask = (1 << PIPE_DRIVE) | (1 << PIPE_FEEDBACK);
const uint8_t pipe_interrupt_mask = (1 << PIPE_KICK) | (1 << PIPE_FAULT_OUT) | (1 << PIPE_FAULT_IN);
const uint8_t pipe_bulk_mask = (1 << PIPE_FIRMWARE_OUT) | (1 << PIPE_FIRMWARE_IN);

