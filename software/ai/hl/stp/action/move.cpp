#include "ai/hl/stp/action/move.h"
#include "ai/flags.h"
#include "ai/hl/stp/action/action.h"
#include "ai/hl/stp/evaluation/rrt_planner.h"
//#include "ai/hl/stp/evaluation/straight_line_planner.h"
#include "ai/hl/stp/evaluation/plan_util.h"
#include "util/dprint.h"

using namespace AI::HL::STP;

DoubleParam default_desired_rpm(
    u8"The default desired rpm for dribbling", u8"AI/Movement/Primitives", 7000,
    0, 100000);

const double MAX_SPEED  = 3.0;
const double SMALL_DIST = 0.1;

static double calc_mid_vel(Point p0, Point p1, Point p2)
{
    Point v1 = p1 - p0;
    Point v2 = p2 - p1;

    // if the length one or both of the vectors are close to zero assume they
    // are collinear
    // if(v2.len() < SMALL_DIST)
    //{
    //	return MAX_SPEED/3.0;
    //}
    // else
    //{
    double angle = std::acos((v1.norm()).dot(v2.norm()));  // should be [0,pi)
    return std::max(0.0, MAX_SPEED * std::cos(angle * angle / 1.5));
    //}
    // return 0.0;
}

// if should_wait is false, robot stops after reaching destination
void AI::HL::STP::Action::move(
    caller_t& ca, World world, Player player, Point dest)
{
    // Default to RRT
    Action::move_rrt(ca, world, player, dest);
    // player.move_move(local_dest(player, dest), Angle(), 0);
    // if(should_wait) Action::wait_move(ca, player, dest);
}

void AI::HL::STP::Action::move(
    caller_t& ca, World world, Player player, Point dest, Angle orientation)
{
    Action::move_rrt(ca, world, player, dest, orientation);
}

// Move in a straight line
void AI::HL::STP::Action::move_straight(
    caller_t& ca, World world, Player player, Point dest)
{
    // Primitive prim = Primitives::Move(player, dest, (world.ball().position()
    // - player.position()).orientation());
    player.move_move(dest, Angle(), 0);
}

void AI::HL::STP::Action::move_straight(
    caller_t& ca, World world, Player player, Point dest, Angle orientation)
{
    player.move_move(dest, orientation, 0);
}

void AI::HL::STP::Action::move_rrt(
    caller_t& ca, World world, Player player, Point dest)
{
    move_rrt(ca, world, player, dest, Angle());
}

void AI::HL::STP::Action::move_rrt(
    caller_t& ca, World world, Player player, Point dest, Angle orientation)
{
    std::vector<Point> way_points;
    std::vector<Point> plan = Evaluation::RRT::rrt_plan(
        world, player, dest, way_points, true, player.flags());
    player.display_path(plan);
    if (!plan.empty())
    {
        if (plan.size() > 1)
        {
            double midVel = calc_mid_vel(player.position(), plan[0], plan[1]);
            player.move_move(plan[0], orientation, midVel);
        }
        else
        {
            player.move_move(plan[0], orientation, 0.0);
        }
    }
}

/*
void AI::HL::STP::Action::move_rrt(caller_t& ca, World world, Player player,
Point dest,  Angle orientation, bool should_wait) {
    //LOG_DEBUG(Glib::ustring::compose("Time for new move robot %3, point %1,
angle %2", local_coord.field_point(), local_coord.field_angle(),
player.pattern()));
//    std::vector<Point> way_points;
//    std::vector<Point> plan = Evaluation::RRT::rrt_plan(world, player, dest,
way_points, true, player.flags());
//
//    for(unsigned int i = 0; i < plan.size(); i++) {
//        player.move_move(plan[i], orientation, 0);
//		while((player.position() - plan[i]).len() > 0.05) {
//			player.display_path(std::vector<Point>(plan.begin() +
i, plan.end()));
//			Action::yield(ca);
//		}
//    }

        std::vector<Point> way_points;
        std::vector<Point> plan = Evaluation::RRT::rrt_plan(world, player, dest,
way_points, true, player.flags());
        player.display_path(plan);
        player.move_move(dest, orientation, 0);

        */
/*It seems like when "spamming" even a little bit, the robot doesn't get within
	 * more than 0.035m of the destination
	 *
	 */ /*
 //    std::vector<Point> way_points;
 //    std::vector<Point> newPlan = Evaluation::RRT::rrt_plan(world, player,
 dest, way_points, true, player.flags());
 //	std::vector<Point> plan = newPlan;
 //	bool loopBroken = false;
 //	int count = 0;
 //
 //	while((player.position() - dest).len() > Robot::MAX_RADIUS/2) {
 //		if(Evaluation::Plan::valid_path(player.position(), dest, world,
 player)) {
 //			LOG_INFO("CAN GO STRAIGHT TO DEST");
 //			player.move_move(dest);
 //			plan.clear();
 //			plan.push_back(dest);
 //		}else {
 //			plan = newPlan;
 //		}
 //
 //		if((plan.back() - dest).len() > 1e-2) {
 //			plan = Evaluation::RRT::rrt_plan(world, player, dest,
 way_points, true, player.flags());
 //		}
 //
 //		plan = newPlan;
 //		loopBroken = false;
 //		count = 0;
 //		for(unsigned int i = 0; i < plan.size(); i++) {
 //			player.display_path(std::vector<Point>(plan.begin() + i,
 plan.end()));
 //			player.move_move(plan[i], orientation, 0);
 //
 //			Action::yield(ca);
 //			while((player.position() - plan[i]).len() >
 Robot::MAX_RADIUS) {
 //				Action::yield(ca);
 //
 player.display_path(std::vector<Point>(plan.begin() + i, plan.end()));
 //
 //				// (plan[i] - Evaluation::RRT::rrt_plan(world,
 player, dest, way_points, true, player.flags())[0]).len() > Robot::MAX_RADIUS *
 2
 //				if(count >= 5) {
 //					newPlan =
 Evaluation::RRT::rrt_plan(world,
 player, dest, way_points, true, player.flags());
 // if(Evaluation::Plan::isNewPathBetter(plan,
 newPlan, dest) ||
 //					   (plan.back() - dest).len() > 1e-6) {
 //						// if a new path is different
 enough from the old path, it's likely better so use it
 //						LOG_INFO(u8"new plan diverged.
 getting new plan");
 //						loopBroken = true;
 //						break;
 //					}
 //				}
 //
 if(!Evaluation::Plan::valid_path(player.position(), plan[i], world, player)) {
 //					// break this path and replan
 //					LOG_INFO(u8"path not valid. getting new
 plan");
 //					newPlan =
 Evaluation::RRT::rrt_plan(world,
 player, dest, way_points, true, player.flags());
 //					loopBroken = true;
 //					break;
 //				}
 //				count++;
 //				LOGF_INFO(u8"remaining points: %1", i <
 plan.size());
 //				LOGF_INFO(u8"dist point, dest: %1, %2",
 (player.position() - plan[i]).len(), (player.position() - dest).len());
 //			}
 //			if(loopBroken) break;
 //			LOG_INFO(u8"Stuck 1");
 //		}
 //		LOG_INFO(u8"Stuck 2");
 //	}
 //	LOG_INFO(u8"Done move_rrt");
 }
 */
void AI::HL::STP::Action::move_slp(
    caller_t& ca, World world, Player player, Point dest)
{
    move_slp(ca, world, player, dest, Angle());
}

void AI::HL::STP::Action::move_slp(
    caller_t& ca, World world, Player player, Point dest, Angle orientation)
{
    move_rrt(ca, world, player, dest, orientation);

        /*std::vector<Point> plan;

	do {
		plan = Evaluation::SLP::straight_line_plan(world, player, dest);
		player.display_path(plan);
		double final_velocity = 2;

		if(plan.size() > 1)
		{
			// Calculate end velocity based on next point, using function 2cos(x^2 / 1.5)
		}

		LOGF_INFO("FINAL VELOCITY: ", final_velocity);
		final_velocity = 0;

		player.move_move(plan[0], orientation, final_velocity);
		yield(ca);
	}while((player.position() - dest).len() > 0.05 && should_wait);
	LOG_INFO("DONE MOVE SLP");
*/}

void AI::HL::STP::Action::move_dribble(
    caller_t& ca, World world, Player player, Angle orientation, Point dest)
{
    std::vector<Point> way_points;
    std::vector<Point> plan = Evaluation::RRT::rrt_plan(
        world, player, dest, way_points, true, player.flags());
    player.display_path(plan);
    for (auto planpt : plan)
    {
        player.move_dribble(planpt, orientation, default_desired_rpm, 0);
    }
}

void AI::HL::STP::Action::move_careful(
    caller_t& ca, World world, Player player, Point dest)
{
    player.flags(player.flags() | AI::Flags::MoveFlags::CAREFUL);
    Action::move(ca, world, player, dest);
}
