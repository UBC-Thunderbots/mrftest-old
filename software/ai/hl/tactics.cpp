#include "ai/flags.h"
#include "ai/hl/tactics.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "uicomponents/param.h"
#include "util/dprint.h"

using namespace AI::HL::W;

namespace {
	DoubleParam lone_goalie_dist("Lone goalie distance to goal post (m)", 0.05, 0.05, 1.0);
}

void AI::HL::Tactics::chase(World &world, Player::Ptr player, const unsigned int flags) {
	player->move(world.ball().position(), (world.ball().position() - player->position()).orientation(), flags, AI::Flags::MOVE_CATCH, AI::Flags::PRIO_HIGH);
}

void AI::HL::Tactics::shoot(World &world, Player::Ptr player, const unsigned int flags, const bool force) {
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
		AI::HL::Tactics::shoot(world, player, flags, target.first);
	}
}

void AI::HL::Tactics::shoot(World &world, Player::Ptr player, const unsigned int flags, const Point target) {
	const double ori_target = (target - player->position()).orientation();

	if (!player->has_ball()) {
		// chase(world, player, flags);
		player->move(world.ball().position(), ori_target, flags, AI::Flags::MOVE_CATCH, AI::Flags::PRIO_HIGH);
		return;
	}

	const double ori_diff = fabs(player->orientation() - ori_target);

	// aim
	player->move(player->position(), ori_target, flags, AI::Flags::MOVE_DRIBBLE, AI::Flags::PRIO_HIGH);

	if (ori_diff > AI::HL::Util::shoot_accuracy * M_PI / 180.0) { // aim
		return;
	}

	// shoot!
	if (player->chicker_ready_time() == 0) {
#warning max power kick for now
		player->kick(1.0);
	}
}

void AI::HL::Tactics::repel(World &world, Player::Ptr player, const unsigned int flags) {
	// set to RAM_BALL instead of using chase
	if (!player->has_ball()) {
		player->move(world.ball().position(), (world.ball().position() - player->position()).orientation(), flags, AI::Flags::MOVE_RAM_BALL, AI::Flags::PRIO_HIGH);
		return;
	}

	// just shoot as long as it's not in backwards direction
	if (player->orientation() < M_PI / 2 && player->orientation() > -M_PI / 2) {
		if (player->chicker_ready_time() == 0) {
			player->kick(1.0);
		}
	}

	// all enemies are obstacles
	/*
	   std::vector<Point> obstacles;
	   EnemyTeam &enemy = world.enemy_team();
	   for (std::size_t i = 0; i < enemy.size(); ++i) {
	   obstacles.push_back(enemy.get(i)->position());
	   }

	   const AI::HL::W::Field &f = world.field();

	   // vertical line at the enemy goal area
	   // basically u want the ball to be somewhere there
	   const Point p1 = Point(f.length() / 2.0, -f.width() / 2.0);
	   const Point p2 = Point(f.length() / 2.0, f.width() / 2.0);
	   std::pair<Point, double> target = angle_sweep_circles(player->position(), p1, p2, obstacles, Robot::MAX_RADIUS);

	   AI::HL::Tactics::shoot(world, player, flags, target.first);
	 */
}

void AI::HL::Tactics::free_move(World &world, Player::Ptr player, const Point p) {
	// no flags
	player->move(p, (world.ball().position() - player->position()).orientation(), 0, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_LOW);
}

void AI::HL::Tactics::lone_goalie(AI::HL::W::World &world, AI::HL::W::Player::Ptr player) {
	const Point default_pos = Point(-0.45 * world.field().length(), 0);
	const Point centre_of_goal = world.field().friendly_goal();
	Point target = world.ball().position() - centre_of_goal;
	target = target * (lone_goalie_dist / target.len());
	target += centre_of_goal;
	player->move(target, (world.ball().position() - player->position()).orientation(), 0, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
}

void AI::HL::Tactics::pass(World &world, Player::Ptr passer, Player::Ptr passee, const unsigned int flags) {
	if (!passee.is()) {
		LOG_ERROR("There is no passee");
		return;
	}

	// this has to be done first
	if (!passer->has_ball()) {
		chase(world, passer, flags);
		return;
	}

	// TODO: if the target is blocked, aim towards the target
	if (!AI::HL::Util::can_receive(world, passee)) {
		const double ori_target = (passee->position() - passer->position()).orientation();
		passer->move(passer->position(), ori_target, flags, AI::Flags::MOVE_DRIBBLE, AI::Flags::PRIO_HIGH);
		return;
	}

	AI::HL::Tactics::shoot(world, passer, flags, passee->position());
}

AI::HL::Tactics::Patrol::Patrol(World &w) : world(w), flags(0) {
	target1 = w.field().friendly_goal();
	target2 = w.field().friendly_goal();
}

void AI::HL::Tactics::Patrol::tick() {
	if (!player.is()) {
		LOG_WARN("No players");
		return;
	}
	if ((player->position() - target1).len() < AI::HL::Util::POS_CLOSE) {
		goto_target1 = false;
	} else if ((player->position() - target2).len() < AI::HL::Util::POS_CLOSE) {
		goto_target1 = true;
	}
	if (goto_target1) {
		player->move(target1, (world.ball().position() - player->position()).orientation(), flags, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
	} else {
		player->move(target2, (world.ball().position() - player->position()).orientation(), flags, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
	}
}

