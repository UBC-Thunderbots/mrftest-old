#ifndef AI_COMMON_OBJECTS_TIME_H
#define AI_COMMON_OBJECTS_TIME_H

#include <chrono>

namespace AI
{
typedef std::chrono::steady_clock::time_point Timestamp;
typedef std::chrono::steady_clock::duration Timediff;
}

#endif
