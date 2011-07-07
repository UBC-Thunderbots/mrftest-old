#include "ai/param.h"

DoubleParam AI::player_average_velocity("Average Player Velocity", "AI/Param", 1.5, 0.01, 99.0);

Temp_AngleParam AI::player_recieve_threshold("Angle in degrees that reciever can be offset", "AI/Param", 1.5, 0.01, 99.0);
