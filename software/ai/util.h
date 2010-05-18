#ifndef AI_UTIL_H
#define AI_UTIL_H

#include "ai/world/world.h"

#include <vector>

namespace ai_util {

//
// Calculates the candidates to aim for when shooting at the goal.
//
const std::vector<point> calc_candidates(const world::ptr w);

//
// Returns an integer i, where candidates[i] is the best point to aim for when shooting.
// Here candidates is the vector returned by calc_candidates.
// If all shots are bad, candidates.size() is returned.
//
size_t calc_best_shot(player::ptr player, const world::ptr w);

}

#endif

