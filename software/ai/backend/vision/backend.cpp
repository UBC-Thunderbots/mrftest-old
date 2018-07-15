#include "ai/backend/vision/backend.h"

DoubleParam AI::BE::Vision::BALL_FILTER_THRESHOLD(
    u8"Ball detection probability threshold", "AI/Backend/Vision", 0.25, 0.0,
    1.0);
BoolParam AI::BE::Vision::DISABLE_VISION_FILTER(
    u8"Disable Vision Filter", u8"AI/Backend/Vision", false);
