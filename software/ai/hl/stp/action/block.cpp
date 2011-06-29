#include "ai/hl/stp/action/block.h"
#include "ai/hl/stp/action/shoot.h"
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

	std::vector<Robot::Ptr> enemies = AI::HL::Util::get_robots(world.enemy_team());
	Robot::Ptr enemy_goalie = *std::min_element(enemies.begin(), enemies.end(), AI::HL::Util::CmpDist<Robot::Ptr>(world.field().enemy_goal()));
	std::vector<Player::CPtr> friendly = AI::HL::Util::get_players(world.friendly_team());
	Player::CPtr baller = *std::min_element(friendly.begin(), friendly.end(), AI::HL::Util::CmpDist<Player::CPtr>(world.ball().position()));

	Point dirToGoal = (world.field().friendly_goal() - robot->position()).norm();

	int block_dist = 1;
	
	// don't block from ball if the enemy is the goalie!!
	if (robot == enemy_goalie) block_dist *= 2;

	Point target = robot->position() + (block_dist * block_threshold * Robot::MAX_RADIUS * dirToGoal);

	// don't block from ball if we are blocking our baller!!
	if (within_angle_thresh(baller, target)) {
		block_dist *= 2;	
		Point target = robot->position() + (block_dist * block_threshold * Robot::MAX_RADIUS * dirToGoal);
	}

	player->move(target, (world.ball().position() - player->position()).orientation(), Point());
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
	Point target = robot->position() + (block_threshold * Robot::MAX_RADIUS * dirToBall);
	std::vector<Player::CPtr> friendly = AI::HL::Util::get_players(world.friendly_team());
	Player::CPtr baller = *std::min_element(friendly.begin(), friendly.end(), AI::HL::Util::CmpDist<Player::CPtr>(world.ball().position()));

	// don't block from ball if we are blocking our baller!!
	if (within_angle_thresh(baller, target)) target = robot->position() + (2 * block_threshold * Robot::MAX_RADIUS * dirToBall);

	player->move(target, (world.ball().position() - player->position()).orientation(), Point());	
	player->type(AI::Flags::MoveType::NORMAL);
	player->prio(AI::Flags::MovePrio::MEDIUM);

}

