#ifndef AI_HL_STP_EVALUATION_INDIRECT_CHERRY_H
#define AI_HL_STP_EVALUATION_INDIRECT_CHERRY_H

#include <array>
#include "ai/hl/stp/play/play.h"
#include "ai/hl/world.h"
#include "util/cacheable.h"
#include "util/param.h"

namespace AI
{
namespace HL
{
namespace STP
{
namespace Evaluation
{
Point cherry_pivot(World world);

bool cherry_at_point(World world, Player player);

/**
 * returns a passee position for passing
 */
}
}
}
}

#endif
