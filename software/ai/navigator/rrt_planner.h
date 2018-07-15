#include <glibmm/nodetree.h>
#include <memory>
#include "ai/navigator/plan.h"

namespace AI
{
namespace MovementPrimitives
{
enum MovePrimType
{
    COAST,
    BRAKE,
    MOVE,
    DRIBBLE,
    SHOOT,
    CATCH,
    PIVOT,
    SPIN
};
}

namespace Nav
{
class RRTPlanner : public Plan
{
   public:
    explicit RRTPlanner(AI::Nav::W::World world);
    virtual std::vector<Point> plan(
        AI::Nav::W::Player player, Point goal,
        AI::Flags::MoveFlags added_flags = AI::Flags::MoveFlags::NONE);

    static constexpr Point empty_state();

   protected:
    /**
     * Determines how far an endpoint in the path is from the goal location
     */
    virtual double distance(Glib::NodeTree<Point> *node, Point goal);

    Point random_point();

    Point choose_target(Point goal, AI::Nav::W::Player player);

    Glib::NodeTree<Point> *nearest(Glib::NodeTree<Point> *tree, Point target);

    /**
     * This function decides how to move toward the target the target is one of
     * a random point,
     * a waypoint, or the goal location. A subclass may override this
     */
    virtual Point extend(
        AI::Nav::W::Player player, Glib::NodeTree<Point> *start, Point target);

    /**
     * This is the useful method in this class it generates a path for a player
     * given the goal
     * optional parameter post_process sets whether to try and smooth out the
     * final path
     */
    std::vector<Point> rrt_plan(
        AI::Nav::W::Player player, Point goal, bool post_process = true,
        AI::Flags::MoveFlags added_flags = AI::Flags::MoveFlags::NONE);
};
}
}

inline constexpr Point AI::Nav::RRTPlanner::empty_state()
{
    return Point(-10000, -10000);
}
