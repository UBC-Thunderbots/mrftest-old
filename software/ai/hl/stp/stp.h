#pragma once

#include <cairomm/context.h>
#include <cairomm/refptr.h>
#include "ai/hl/stp/world.h"

namespace AI
{
namespace HL
{
namespace STP
{
/**
 * Run all evaluations.
 */
void tick_eval(World world);
void stop_threads();

void draw_ui(World world, Cairo::RefPtr<Cairo::Context> ctx);

Player get_goalie();
}
}
}
