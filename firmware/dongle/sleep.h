#ifndef SLEEP_H
#define SLEEP_H

void sleep_systick_overflows(unsigned long ticks);

#define sleep_millis(ms) sleep_systick_overflows((ms) * 10U)
#define sleep_100us sleep_systick_overflows

#endif

