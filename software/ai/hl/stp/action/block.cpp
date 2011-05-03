#include "ai/hl/stp/action/block.h"
#include "ai/flags.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace AI::HL::STP;

void AI::HL::STP::Action::block(const World &world, Player::Ptr player, Robot::Ptr robot) {

	// should have threshold distance, half a robot radius?

	//Point near_enemy(enemy->evaluate()->position().x - Robot::MAX_RADIUS * 3, enemy->evaluate()->position().y);
	//player->move(near_enemy, (world.ball().position() - player->position()).orientation(), AI::Flags::calc_flags(world.playtype()), AI::Flags::MoveType::NORMAL, AI::Flags::MovePrio::MEDIUM);

	Point dirToGoal = (world.field().friendly_goal() - robot->position()).norm();
	player->move(robot->position() + (0.5*Robot::MAX_RADIUS*dirToGoal), (world.ball().position() - player->position()).orientation(), Point());
	player->type(AI::Flags::MoveType::NORMAL);
}

void AI::HL::STP::Action::block_pass(const World &world, Player::Ptr player, Robot::Ptr robot) {

	// should have threshold distance, half a robot radius?
	Point dirToBall = (world.ball().position() - robot->position()).norm();
	player->move(robot->position() + (0.5*Robot::MAX_RADIUS*dirToBall), (world.ball().position() - player->position()).orientation(), Point());
	
	player->type(AI::Flags::MoveType::NORMAL);
	player->prio(AI::Flags::MovePrio::MEDIUM);
}

