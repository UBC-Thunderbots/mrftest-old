#include "ai/flags.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/action/action.h"
#include "ai/hl/stp/evaluation/rrt_planner.h"

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
    std::vector<Point> way_points;
    std::vector<Point> plan = Evaluation::RRT::rrt_plan(world, player, dest, way_points, true, player.flags()); 
    player.display_path(plan);
    for(auto planpt : plan){
        //player.move_move(local_dest(player, dest), orientation - player.orientation(), 0);
        // use global
        player.move_move(planpt, Angle(), 0);
        LOGF_INFO(u8"before wait move: Current position:%1, Dest:%2, planpt: %3", player.position(), dest, planpt);
        if(should_wait) Action::wait_move(ca, player, planpt);

    }
}

void AI::HL::STP::Action::move_rrt(caller_t& ca, World world, Player player, Point dest,  Angle orientation, bool should_wait) {
    //LOG_DEBUG(Glib::ustring::compose("Time for new move robot %3, point %1, angle %2", local_coord.field_point(), local_coord.field_angle(), player.pattern()));
    std::vector<Point> way_points;
    std::vector<Point> plan = Evaluation::RRT::rrt_plan(world, player, dest, way_points, true, player.flags()); 
    player.display_path(plan);
    for(auto planpt : plan){
        player.move_move(planpt, orientation, 0);
        if(should_wait) Action::wait_move(ca, player, planpt);
    }
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

