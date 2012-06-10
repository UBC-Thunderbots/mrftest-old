#ifndef SLEEP_H
#define SLEEP_H

void sleep_systick_overflows(unsigned long ticks);

#define sleep_1ms(x) sleep_100us((x) * 10UL)
#define sleep_100us(x) sleep_10us((x) * 10UL)
#define sleep_10us(x) sleep_1us((x) * 10UL)
#define sleep_1us sleep_systick_overflows

#endif

