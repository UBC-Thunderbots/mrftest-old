#include "ai/flags.h"
#include "ai/hl/stp/action/actions.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "uicomponents/param.h"
#include "util/dprint.h"

using namespace AI::HL::STP;

namespace {
	DoubleParam lone_goalie_dist("Lone goalie distance to goal post (m)", 0.30, 0.05, 1.0);
}

void AI::HL::STP::Actions::chase(const World &world, Player::Ptr player, const unsigned int flags) {
	player->move(world.ball().position(), (world.ball().position() - player->position()).orientation(), flags, AI::Flags::MOVE_CATCH, AI::Flags::PRIO_HIGH);
}

void AI::HL::STP::Actions::shoot(const World &world, Player::Ptr player, const unsigned int flags, const bool force) {
	std::pair<Point, double> target = AI::HL::Util::calc_best_shot(world, player);

	if (!player->has_ball()) {
		if (target.second == 0) {
			chase(world, player, flags);
		} else {
			player->move(world.ball().position(), (world.field().enemy_goal() - player->position()).orientation(), flags, AI::Flags::MOVE_CATCH, AI::Flags::PRIO_HIGH);
		}
		return;
	}

	if (target.second == 0) { // blocked
		if (force) {
			// TODO: perhaps do a reduced radius calculation
			target.first = world.field().enemy_goal();
			shoot(world, player, flags, target.first);
		} else { // just aim at the enemy goal
			player->move(player->position(), (world.field().enemy_goal() - player->position()).orientation(), flags, AI::Flags::MOVE_DRIBBLE, AI::Flags::PRIO_HIGH);
		}
	} else {
		// call the other shoot function with the specified target
		AI::HL::STP::Actions::shoot(world, player, flags, target.first);
	}
}

void AI::HL::STP::Actions::shoot(const World &world, Player::Ptr player, const unsigned int flags, const Point target, const double kick_power) {
	const double ori_target = (target - player->position()).orientation();

	if (!player->has_ball()) {
		// chase(world, player, flags);
		player->move(world.ball().position(), ori_target, flags, AI::Flags::MOVE_CATCH, AI::Flags::PRIO_HIGH);
		return;
	}

	// const double ori_diff = std::fabs(player->orientation() - ori_target);

	// aim
	player->move(player->position(), ori_target, flags, AI::Flags::MOVE_DRIBBLE, AI::Flags::PRIO_HIGH);

	// ignoring accuracy, comment this out for now so that we'll shoot more
	// if (ori_diff > AI::HL::STP::Util::shoot_accuracy * M_PI / 180.0) { // aim
	// return;
	// }

	// shoot!
	if (player->chicker_ready_time() == 0) {
		player->kick(kick_power);
	}
}

void AI::HL::STP::Actions::repel(const World &world, Player::Ptr player, const unsigned int flags) {
	// set to RAM_BALL instead of using chase
	if (!player->has_ball()) {
		player->move(world.ball().position(), (world.ball().position() - player->position()).orientation(), flags, AI::Flags::MOVE_RAM_BALL, AI::Flags::PRIO_HIGH);
		return;
	}

	// just shoot as long as it's not in backwards direction
	if (player->orientation() < M_PI / 2 && player->orientation() > -M_PI / 2) {
		if (player->chicker_ready_time() == 0) {
			player->kick(10.0);
		}
	}

	player->move(world.ball().position(), (world.ball().position() - player->position()).orientation(), flags, AI::Flags::MOVE_RAM_BALL, AI::Flags::PRIO_HIGH);

	// all enemies are obstacles
	/*
	   std::vector<Point> obstacles;
	   EnemyTeam &enemy = world.enemy_team();
	   for (std::size_t i = 0; i < enemy.size(); ++i) {
	   obstacles.push_back(enemy.get(i)->position());
	   }

	   const Field &f = world.field();

	   // vertical line at the enemy goal area
	   // basically u want the ball to be somewhere there
	   const Point p1 = Point(f.length() / 2.0, -f.width() / 2.0);
	   const Point p2 = Point(f.length() / 2.0, f.width() / 2.0);
	   std::pair<Point, double> target = angle_sweep_circles(player->position(), p1, p2, obstacles, Robot::MAX_RADIUS);

	   AI::HL::STP::Actions::shoot(world, player, flags, target.first);
	 */
}

void AI::HL::STP::Actions::free_move(const World &world, Player::Ptr player, const Point p) {
	player->move(p, (world.ball().position() - player->position()).orientation(), AI::Flags::calc_flags(world.playtype()), AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_LOW);
}

void AI::HL::STP::Actions::lone_goalie(const World &world, Player::Ptr player) {
	// if ball is inside the defense area, must repel!
	if (AI::HL::Util::point_in_friendly_defense(world.field(), world.ball().position())) {
		repel(world, player, 0);
		return;
	}

	const Point default_pos = Point(-0.45 * world.field().length(), 0);
	const Point centre_of_goal = world.field().friendly_goal();
	Point target = world.ball().position() - centre_of_goal;
	target = target * (lone_goalie_dist / target.len());
	target += centre_of_goal;
	player->move(target, (world.ball().position() - player->position()).orientation(), 0, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
}

void AI::HL::STP::Actions::block(World &world, Player::Ptr player, const unsigned int flags, Robot::Ptr robot) {
#warning can be better
	player->move(robot->position(), (world.ball().position() - player->position()).orientation(), flags, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
}

