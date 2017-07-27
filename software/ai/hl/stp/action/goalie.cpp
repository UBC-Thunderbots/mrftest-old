#include "goalie.h"

#include "ai/hl/stp/action/action.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/flags.h"
#include "ai/hl/stp/action/ram.h"
#include "ai/hl/stp/evaluation/ball_threat.h"
#include "ai/hl/util.h"
#include "chip.h"
#include "geom/util.h"
#include "repel.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace AI::HL::STP;
using AI::HL::STP::Evaluation::BallThreat;

namespace {
	const double FAST = 100.0;
	DoubleParam lone_goalie_dist(u8"Lone Goalie: distance to goal post (m)", u8"AI/HL/STP/Action/Goalie", 0.30, 0.05, 1.0);
	DoubleParam goalie_repel_dist(u8"Distance the goalie should repel the ball in robot radius", u8"AI/HL/STP/Action/Goalie", 4.0, 1.0, 6.0);
}

//goalie moves in the direction towards the ball within the lone_goalie_dist from the goal post
void AI::HL::STP::Action::lone_goalie(caller_t& ca, World world, Player player) {
	for (auto i : world.enemy_team()) {
	// If enemy is in our defense area, go touch them so we get penalty kick
		if(AI::HL::Util::point_in_friendly_defense(world.field() , i.position())) {
			player.avoid_distance(AI::Flags::AvoidDistance::SHORT);
			Action::move(ca, world, player, i.position());
			return;
		}
	}
	// Patrol
	//const Point default_pos = Point(-0.45 * world.field().length(), 0);
	const Point centre_of_goal = world.field().friendly_goal();
	Point target = world.ball().position() - centre_of_goal;

	double lone_goalie_dist2 = (world.field().friendly_goal_boundary().first - world.field().friendly_goal_boundary().second).len() / 2 + 0.05;

	target = target.norm() * lone_goalie_dist2;
	target += centre_of_goal;
	if (target.x < world.field().friendly_goal().x + Robot::MAX_RADIUS) {
		// avoid going inside the goal
		target.x = world.field().friendly_goal().x + Robot::MAX_RADIUS;
	}

	double r = world.field().defense_area_radius() + world.field().defense_area_stretch() / 2;
	// prevents goalie from chasing ball too far??
	if ((target - centre_of_goal).len() > r) {
		target = centre_of_goal + (target - centre_of_goal).norm() * r;
	}
	goalie_move(ca, world, player, target);
}

void AI::HL::STP::Action::goalie_move(caller_t& ca, World world, Player player, Point dest) {

	for (auto i : world.enemy_team()) {
	// If enemy is in our defense area, go touch them so we get penalty kick
		if(AI::HL::Util::point_in_friendly_defense(world.field() , i.position())) {
			player.avoid_distance(AI::Flags::AvoidDistance::SHORT);
			player.move_move(i.position());
			return;
		}
	}
	player.move_move(dest, (world.ball().position() - player.position()).orientation());
	return;

	// if ball is inside the defense area or just too close, repel!!!!
	if ((AI::HL::Util::point_in_friendly_defense(world.field(), world.ball().position()) ||
		(world.ball().position() - player.position()).len() < goalie_repel_dist * Robot::MAX_RADIUS) &&
		world.playtype() != AI::Common::PlayType::STOP)
	{
		goalie_repel(ca, world, player);
		return;
	}
	// check if ball is heading towards our goal
	if (Evaluation::ball_on_net(world)) {
		// goalie block position
		Point goal_pos = Evaluation::goalie_shot_block(world, player);
		// avoid going inside of goal
		if (goal_pos.x < world.field().friendly_goal().x + Robot::MAX_RADIUS) {
					// avoid going outside the goal??
			goal_pos.x = world.field().friendly_goal().x + Robot::MAX_RADIUS;
		}

		goalie_ram(ca, world, player, goal_pos);

	} else {
//		if(pow( pow(dest.x, 2) + pow(dest.y , 2) , 0.5 ) > 0.5 - Robot::MAX_RADIUS) {
//			double angle = atan(dest.y/dest.x);
//			dest.x = 0.3*cos(angle) + world.fi;
//			dest.y = 0.3*sin(angle);
//		}
//		if ((goal_center - dest).len() > 0.5) {
//			dest = goal_center + (dest - goal_center).norm() * 0.5;
//		}
		goalie_ram(ca, world, player, dest);
	}
}

void AI::HL::STP::Action::goalie_move_direct(caller_t& ca, World world, Player player, const Point dest) {
	Action::move(ca, world, player, dest, (world.ball().position() - player.position()).orientation());
}
