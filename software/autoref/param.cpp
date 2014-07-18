#include "ai/param.h"

DoubleParam AI::player_average_velocity(u8"Average Player Velocity", u8"AI/Param", 1.5, 0.01, 99.0);

DegreeParam AI::player_receive_threshold(u8"Angle that receiver can be offset (degrees)", u8"AI/Param", 1.5, 0.01, 99.0);

