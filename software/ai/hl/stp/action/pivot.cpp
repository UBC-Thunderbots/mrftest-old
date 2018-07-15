#include "ai/hl/stp/action/pivot.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include "ai/flags.h"
#include "ai/hl/stp/action/intercept.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/action/pivot.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/plan_util.h"
#include "ai/hl/stp/world.h"
#include "ai/hl/util.h"
#include "ai/hl/world.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace AI::HL::STP;
using namespace AI::HL::W;
namespace Plan = AI::HL::STP::Evaluation::Plan;

namespace
{
DoubleParam DEFAULT_FINAL_VEL(
    u8"the default final velocity for points generated while pivoting",
    u8"AI/HL/STP/Action/pivot", 0.75, 0.0, 5.0);
IntParam ROTATION(
    u8"Amount of rotation for each point (in degrees)",
    u8"AI/HL/STP/Action/pivot", 10, 0, 10);
}

// TODO: Test this now that it should be stateless
void AI::HL::STP::Action::pivot(
    caller_t& ca, World world, Player player, Point target, Angle finalAngle,
    double radius)
{
    AI::BE::Primitives::Ptr prim(
        new Primitives::Pivot(player, target, finalAngle, radius));
    (static_cast<AI::Common::Player>(player)).impl->push_prim(prim);
}
