#include "ai/flags.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/action/action.h"
#include "ai/hl/stp/evaluation/rrt_planner.h"

using namespace AI::HL::STP;

// if should_wait is false, robot stops after reaching destination
void AI::HL::STP::Action::move(caller_t& ca, World world, Player player, Point dest, bool should_wait) {
    LOG_INFO(Glib::ustring::compose("Time for new move, point %1", dest-player.position()));
    //Angle angle = (world.ball().position() - player.position()).orientation();
    // Simply construct a path with the planner and keep on following it
    // TODO: Update the t_delay when its actually implemented
    player.move_move(local_dest(player, dest), Angle(), 0);
    //Primitive prim = Primitives::Move(player, dest, (world.ball().position() - player.position()).orientation());
	if(should_wait) Action::wait_move(ca, player, dest);
}

void AI::HL::STP::Action::move(caller_t& ca, World world, Player player, Point dest,  Angle orientation, bool should_wait) {
    //LOG_DEBUG(Glib::ustring::compose("Time for new move robot %3, point %1, angle %2", local_coord.field_point(), local_coord.field_angle(), player.pattern()));
    player.move_move(local_dest(player, dest), orientation - player.orientation(), 0);
	//Primitive prim = Primitives::Move(player, dest, orientation);
	if(should_wait) Action::wait_move(ca, player, dest, orientation);
}

void AI::HL::STP::Action::move_rrt(caller_t& ca, World world, Player player, Point dest, bool should_wait) {
    LOG_INFO(Glib::ustring::compose("Time for new move, point %1", dest-player.position()));
    // Simply construct a path with the planner and keep on following it
    std::vector<Point> way_points;
    std::vector<Point> plan = Evaluation::RRT::rrt_plan(world, player, dest, way_points, true, player.flags()); 
    player.display_path(plan);
    for(auto planpt : plan){
        player.move_move(planpt, Angle(), 0);
        if(should_wait) Action::wait_move(ca, player, planpt);
    }
}

void AI::HL::STP::Action::move_rrt(caller_t& ca, World world, Player player, Point dest,  Angle orientation, bool should_wait) {
    //LOG_DEBUG(Glib::ustring::compose("Time for new move robot %3, point %1, angle %2", local_coord.field_point(), local_coord.field_angle(), player.pattern()));
    std::vector<Point> way_points;
    std::vector<Point> plan = Evaluation::RRT::rrt_plan(world, player, dest, way_points, true, player.flags()); 
    player.display_path(plan);
    for(auto planpt : plan){
        player.move_move(dest, orientation, 0);
        if(should_wait) Action::wait_move(ca, player, planpt);
    }
}

void move_dribble(caller_t& ca, World world, Player player, Angle orientation, Point dest, bool should_wait) {
	Primitive prim = Primitives::Dribble(player, dest, orientation, false);
	if(should_wait) Action::wait_move(ca, player, dest, orientation);
}

void AI::HL::STP::Action::move_careful(caller_t& ca, World world, Player player, Point dest, bool should_wait) {
	player.flags(player.flags() | AI::Flags::MoveFlags::CAREFUL);

	Primitive prim = Primitives::Move(player, dest, (world.ball().position() - player.position()).orientation());
	if(should_wait) Action::wait_move(ca, player, dest);
}

