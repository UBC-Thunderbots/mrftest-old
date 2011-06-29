#include "ai/hl/stp/param.h"

DoubleParam AI::HL::STP::min_shoot_region("minimum region available for baller_can_shoot to be true (degrees)", "STP/param", 0.1 / M_PI * 180.0, 0, 180);

