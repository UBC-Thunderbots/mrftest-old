#ifndef ESTOP_H
#define ESTOP_H

typedef enum {
	ESTOP_BROKEN,
	ESTOP_STOP,
	ESTOP_RUN,
} estop_t;

void estop_init(void);

estop_t estop_read(void);

void estop_set_change_callback(void (*cb)(void));

#endif

