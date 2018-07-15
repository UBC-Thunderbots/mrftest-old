#include "ai/navigator/plan.h"

namespace AI
{
namespace Nav
{
class SplinePlanner : public Plan
{
   public:
    explicit SplinePlanner(AI::Nav::W::World world);
    virtual std::vector<Point> plan(
        AI::Nav::W::Player player, Point goal,
        AI::Flags::MoveFlags added_flags = AI::Flags::MoveFlags::NONE);

};
}
}
