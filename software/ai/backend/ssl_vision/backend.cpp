#include "ai/backend/ssl_vision/backend.h"

DoubleParam AI::BE::SSLVision::BALL_FILTER_THRESHOLD(u8"Ball detection probability threshold", "AI/Backend/Vision", 0.25, 0.0, 1.0);
BoolParam AI::BE::SSLVision::USE_PARTICLE_FILTER(u8"Disables the Particle filter if unchecked.", u8"AI/Backend/Vision", true);
