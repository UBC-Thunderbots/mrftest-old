#include "ai/navigator/navigator.h"
#include "ai/navigator/rrt_physics_planner.h"
#include "ai/navigator/util.h"
#include "util/dprint.h"
#include "util/timestep.h"

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav;
using namespace AI::Nav::W;
using namespace AI::Nav::Util;
using namespace AI::Flags;
using namespace Glib;

namespace
{
constexpr double MAX_SPEED = 2.0;
constexpr double THRESHOLD = 0.07;
// const double STEP_DISTANCE = 0.3;
constexpr double TIMESTEP     = 1.0 / TIMESTEPS_PER_SECOND;
constexpr double VALID_REGION = 0.3 * (9 / 8) * TIMESTEP * TIMESTEP;
// probability that we will take a step towards the goal
constexpr double GOAL_PROB     = 0.1;
constexpr double WAYPOINT_PROB = 0.6;
constexpr double RAND_PROB     = 1.0 - GOAL_PROB - WAYPOINT_PROB;

class RRTPhysicsNavigator final : public Navigator
{
   public:
    explicit RRTPhysicsNavigator(World world);
    void tick() override;
    NavigatorFactory &factory() const override;

   private:
    PhysicsPlanner planner;
};
}

RRTPhysicsNavigator::RRTPhysicsNavigator(World world)
    : Navigator(world), planner(world)
{
}

void RRTPhysicsNavigator::tick()
{
    LOG_ERROR("RRT physics doesn't work with MPs!");
    /*
    const AI::Timestamp currentTime = world.monotonic_time();
    for (Player player : world.friendly_team()) {
            const double dist = (player.position() -
    player.destination().first).len();
            AI::Timestamp finalTime = currentTime +
    std::chrono::duration_cast<AI::Timediff>(std::chrono::duration<double>(dist
    / MAX_SPEED));

            std::vector<Point> pathPoints = planner.plan(player,
    player.destination().first);

            Angle destOrientation = player.destination().second;
            Player::Path path;
            for (std::size_t j = 0; j < pathPoints.size(); ++j) {
                    // the last point will just use whatever the last
    orientation was
                    if (j + 1 != pathPoints.size()) {
                            destOrientation = (pathPoints[j + 1] -
    pathPoints[j]).orientation();
                    }

                    path.push_back(std::make_pair(std::make_pair(pathPoints[j],
    destOrientation), finalTime));
            }

            // just use the current player position as the destination if we are
    within the
            // threshold already
            if (pathPoints.empty()) {
                    path.push_back(std::make_pair(std::make_pair(player.position(),
    destOrientation), finalTime));
            }

            player.path(path);
    }
    */
}

NAVIGATOR_REGISTER(RRTPhysicsNavigator)
