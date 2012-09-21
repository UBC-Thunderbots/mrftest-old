#include "ai/hl/stp/action/goalie.h"
#include "ai/hl/stp/action/repel.h"
#include "ai/hl/stp/action/ram.h"
#include "ai/hl/stp/evaluation/ball_threat.h"
#include "ai/flags.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace AI::HL::STP;

namespace Evaluation = AI::HL::STP::Evaluation;
using AI::HL::STP::Evaluation::BallThreat;

namespace {
	const double FAST = 100.0;
	DoubleParam lone_goalie_dist("Lone Goalie: distance to goal post (m)", "STP/Action/Goalie", 0.30, 0.05, 1.0);
	DoubleParam goalie_repel_dist("Distance the defender should repel the ball in robot radius", "STP/Action/Goalie", 4.0, 1.0, 6.0);
}

//goalie moves in the direction towards the ball within the lone_goalie_dist from the goal post
void AI::HL::STP::Action::lone_goalie(World world, Player::Ptr player) {
	// Patrol
	//const Point default_pos = Point(-0.45 * world.field().length(), 0);
	const Point centre_of_goal = world.field().friendly_goal();
	Point target = world.ball().position() - centre_of_goal;
	target = target * (lone_goalie_dist / target.len());
	target += centre_of_goal;
	if (target.x < world.field().friendly_goal().x + Robot::MAX_RADIUS) { // avoid going inside the goal
		target.x = world.field().friendly_goal().x + Robot::MAX_RADIUS;
	}

	goalie_move(world, player, target);
}

void AI::HL::STP::Action::goalie_move(World world, Player::Ptr player, Point dest) {
	//autokick always on, if player has chipper, its autochip
	if (player->has_chipper()) {
		player->autochip(1);
	} else {
		player->autokick(BALL_MAX_SPEED);
	}

	// if ball is inside the defense area or just too close, repel!!!!
	if ((AI::HL::Util::point_in_friendly_defense(world.field(), world.ball().position()) || (world.ball().position() - player->position()).len() < goalie_repel_dist * Robot::MAX_RADIUS) && world.playtype() != AI::Common::PlayType::STOP) {
		repel(world, player);
		return;
	}

	const Point diff = world.ball().position() - player->position();
	// check if ball is heading towards our goal
	if (Evaluation::ball_on_net(world)) {
		// goalie block position
		Point goal_pos = Evaluation::goalie_shot_block(world, player);
		if (goal_pos.x < world.field().friendly_goal().x + Robot::MAX_RADIUS) { // avoid going inside the goal
			goal_pos.x = world.field().friendly_goal().x + Robot::MAX_RADIUS;
		}
		ram(world, player, goal_pos, diff.norm() * FAST);
	} else {
		ram(world, player, dest, diff.norm() * FAST);
	}
}

