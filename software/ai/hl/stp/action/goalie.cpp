#include "ai/hl/stp/action/goalie.h"
#include "ai/flags.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/action/actions.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace AI::HL::STP;

namespace {
	DoubleParam lone_goalie_dist("Lone Goalie: distance to goal post (m)", "STP/Action/Goalie" , 0.30, 0.05, 1.0);
	DoubleParam ball_dangerous_speed("Goalie Action: threatening ball speed", "STP/Action/Goalie", 0.1, 0.1, 10.0); 
}

void AI::HL::STP::Action::lone_goalie(const World &world, Player::Ptr player) {

	// Patrol
	const Point default_pos = Point(-0.45 * world.field().length(), 0);
	const Point centre_of_goal = world.field().friendly_goal();
	Point target = world.ball().position() - centre_of_goal;
	target = target * (lone_goalie_dist / target.len());
	target += centre_of_goal;
	if (target.x < -world.field().length()/2+0.2) { // avoid going inside the goal
		target.x = -world.field().length()/2+0.2;
	}
	player->move(target, (world.ball().position() - player->position()).orientation(), Point());
	player->type(AI::Flags::MoveType::NORMAL);
	player->prio(AI::Flags::MovePrio::MEDIUM);

	goalie_move(world, player, target);
}

void AI::HL::STP::Action::goalie_move(const World &world, Player::Ptr player, Point dest) {
	// if ball is inside the defense area, must repel!
	if (AI::HL::Util::point_in_friendly_defense(world.field(), world.ball().position())) {
		repel(world, player);
		return;
	}

	// Check if ball is threatening to our goal
	Point ballvel = world.ball().velocity();
	Point ballpos = world.ball().position();
	Point goalpos;
	if (ballvel.len() > ball_dangerous_speed && ballvel.x < -1e-6) {

		goalpos = line_intersect(ballpos, ballpos + ballvel, 
				Point(-world.field().length()/2.0+0.2, 1.0),
				Point(-world.field().length()/2.0+0.2, -1.0));

		if (std::fabs(goalpos.y) < world.field().goal_width()/2.0) {
			player->move(goalpos, (world.ball().position() - player->position()).orientation(), Point());
			player->type(AI::Flags::MoveType::RAM_BALL);
			player->prio(AI::Flags::MovePrio::HIGH);
			return;
		}
	}

	player->move(dest, (world.ball().position() - player->position()).orientation(), Point());
	player->type(AI::Flags::MoveType::NORMAL);
	player->prio(AI::Flags::MovePrio::MEDIUM);
}

