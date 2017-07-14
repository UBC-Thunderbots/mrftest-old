#include "ai/flags.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/action/action.h"
#include "ai/hl/stp/evaluation/rrt_planner.h"
#include "ai/hl/stp/evaluation/plan_util.h"
#include "util/dprint.h"

using namespace AI::HL::STP;


DoubleParam default_desired_rpm(u8"The default desired rpm for dribbling", u8"AI/Movement/Primitives", 7000, 0, 100000);

// if should_wait is false, robot stops after reaching destination
void AI::HL::STP::Action::move(caller_t& ca, World world, Player player, Point dest, bool should_wait) {
    // Default to RRT
    Action::move_rrt(ca, world, player, dest, should_wait);
    //player.move_move(local_dest(player, dest), Angle(), 0);
	//if(should_wait) Action::wait_move(ca, player, dest);
}

void AI::HL::STP::Action::move(caller_t& ca, World world, Player player, Point dest,  Angle orientation, bool should_wait) {
    Action::move_rrt(ca, world, player, dest, orientation, should_wait);
}

// Move in a straight line
void AI::HL::STP::Action::move_straight(caller_t& ca, World world, Player player, Point dest, bool should_wait) {
    //Primitive prim = Primitives::Move(player, dest, (world.ball().position() - player.position()).orientation());
    player.move_move(dest, Angle(), 0);
	if(should_wait) Action::wait_move(ca, player, dest);
}

void AI::HL::STP::Action::move_straight(caller_t& ca, World world, Player player, Point dest,  Angle orientation, bool should_wait) {
    player.move_move(dest, orientation, 0);
	if(should_wait) Action::wait_move(ca, player, dest, orientation);
}

void AI::HL::STP::Action::move_rrt(caller_t& ca, World world, Player player, Point dest, bool should_wait) {
	move_rrt(ca, world, player, dest, Angle(), should_wait);
}

void AI::HL::STP::Action::move_rrt(caller_t& ca, World world, Player player, Point dest,  Angle orientation, bool should_wait) {
    //LOG_DEBUG(Glib::ustring::compose("Time for new move robot %3, point %1, angle %2", local_coord.field_point(), local_coord.field_angle(), player.pattern()));
//    std::vector<Point> way_points;
//    std::vector<Point> plan = Evaluation::RRT::rrt_plan(world, player, dest, way_points, true, player.flags());
//
//    for(unsigned int i = 0; i < plan.size(); i++) {
//        player.move_move(plan[i], orientation, 0);
//		while((player.position() - plan[i]).len() > 0.05) {
//			player.display_path(std::vector<Point>(plan.begin() + i, plan.end()));
//			Action::yield(ca);
//		}
//    }

    std::vector<Point> way_points;
    std::vector<Point> newPlan = Evaluation::RRT::rrt_plan(world, player, dest, way_points, true, player.flags());
	std::vector<Point> plan = newPlan;
	bool replan = false;
	bool loopBroken = false;
	int count = 0;

	while((player.position() - dest).len() > 0.05) {
		plan = Evaluation::RRT::rrt_plan(world, player, dest, way_points, true, player.flags());

		loopBroken = false;
		count = 0;
		for(unsigned int i = 0; i < plan.size(); i++) {
			player.display_path(std::vector<Point>(plan.begin() + i, plan.end()));
			player.move_move(plan[i], orientation, 0);

			while((player.position() - plan[i]).len() > 0.04) {
				Action::yield(ca);
				player.display_path(std::vector<Point>(plan.begin() + i, plan.end()));
				if(count >= 5 && (plan[i] - Evaluation::RRT::rrt_plan(world, player, dest, way_points, true, player.flags())[0]).len() > 0.1) {
					// if a new path is different enough from the old path, it's likely better so use it
					LOG_INFO(u8"new plan diverged. getting new plan");
					loopBroken = true;
					break;
				}
				if(!Evaluation::Plan::valid_path(player.position(), plan[i], world, player)) {
					// break this path and replan
					LOG_INFO(u8"path not valid. getting new plan");
					loopBroken = true;
					break;
				}
				count++;
			}
			if(loopBroken) break;
		}
	}
	LOG_INFO(u8"Done move_rrt");
}

void AI::HL::STP::Action::move_dribble(caller_t& ca, World world, Player player, Angle orientation, Point dest, bool should_wait) {
    std::vector<Point> way_points;
    std::vector<Point> plan = Evaluation::RRT::rrt_plan(world, player, dest, way_points, true, player.flags()); 
    player.display_path(plan);
    for(auto planpt : plan){
        player.move_dribble(planpt, orientation, default_desired_rpm, 0);
        if(should_wait) Action::wait_move(ca, player, planpt);
    }
}

void AI::HL::STP::Action::move_careful(caller_t& ca, World world, Player player, Point dest, bool should_wait) {
	player.flags(player.flags() | AI::Flags::MoveFlags::CAREFUL);
    Action::move(ca, world, player, dest, should_wait);  
}

