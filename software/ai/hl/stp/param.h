#pragma once
#include "geom/param.h"
#include "util/param.h"

namespace AI
{
namespace HL
{
namespace STP
{
extern DoubleParam min_pass_dist;

extern DegreeParam min_shoot_region;

extern DegreeParam passee_angle_threshold;

extern RadianParam shoot_accuracy;

extern DoubleParam shoot_width;

extern DoubleParam goal_avoid_radius;

namespace Action
{
// these are pass specific
// needs comments
extern DoubleParam alpha;

extern DoubleParam pass_speed;

extern DegreeParam passer_angle_threshold;

extern DoubleParam target_region_param;
}

namespace Tactic
{
extern BoolParam random_penalty_goalie;

extern DegreeParam separation_angle;
}
}
}
}
