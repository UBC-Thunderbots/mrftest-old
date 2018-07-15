#pragma once

#include "ai/navigator/world.h"
#include "geom/point.h"

namespace AI
{
namespace Nav
{
class Plan
{
   protected:
    AI::Nav::W::World world;

   public:
    explicit Plan(AI::Nav::W::World world);
    virtual std::vector<Point> plan(
        AI::Nav::W::Player player, Point goal,
        AI::Flags::MoveFlags added_flags = AI::Flags::MoveFlags::NONE) = 0;
};
}
}
