#ifndef LPS_H
#define LPS_H

#include "lps.h"
#include <stdint.h>

#define LPS_ARRAY_SIZE 4

typedef float lps_values[LPS_ARRAY_SIZE];
typedef float lps_adc[16];

void lps_init(void);
void lps_incr(void);
int lps_get(void);

#endif
