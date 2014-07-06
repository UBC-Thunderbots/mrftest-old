#ifndef BREAKBEAM_H
#define BREAKBEAM_H

#include <stdbool.h>

float breakbeam_difference(void);
bool breakbeam_interrupted(void);
void breakbeam_tick(void);
void breakbeam_tick_fast(void);

#endif

