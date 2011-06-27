#include "ai/hl/stp/action/block.h"
#include "ai/flags.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace AI::HL::STP;

namespace {
	DoubleParam block_threshold("block threshold distance in terms of robot radius", "STP/Action/block", 3.0, 2.0, 8.0);

}

void AI::HL::STP::Action::block_goal(const World &world, Player::Ptr player, Robot::Ptr robot) {

	Point dirToGoal = (world.field().friendly_goal() - robot->position()).norm();
	player->move(robot->position() + (block_threshold * Robot::MAX_RADIUS * dirToGoal), (world.ball().position() - player->position()).orientation(), Point());
	player->type(AI::Flags::MoveType::NORMAL);
	player->prio(AI::Flags::MovePrio::MEDIUM);

}

void AI::HL::STP::Action::block_ball(const World &world, Player::Ptr player, Robot::Ptr robot) {

	std::vector<Robot::Ptr> enemies = AI::HL::Util::get_robots(world.enemy_team());
	Robot::Ptr enemy_goalie = *std::min_element(enemies.begin(), enemies.end(), AI::HL::Util::CmpDist<Robot::Ptr>(world.field().enemy_goal()));

	// don't block from ball if the enemy is the goalie!!
	if (robot == enemy_goalie) {
		return block_goal(world, player, robot); 
	}

	Point dirToBall = (world.ball().position() - robot->position()).norm();
	player->move(robot->position() + (block_threshold * Robot::MAX_RADIUS * dirToBall), (world.ball().position() - player->position()).orientation(), Point());
	player->type(AI::Flags::MoveType::NORMAL);
	player->prio(AI::Flags::MovePrio::MEDIUM);

}

