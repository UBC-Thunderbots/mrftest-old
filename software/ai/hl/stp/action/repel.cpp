#include "ai/hl/stp/action/repel.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/ram.h"
#include "ai/flags.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "util/param.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/stp/predicates.h"

using namespace AI::HL::STP;

namespace {
	const double FAST = 100.0;
	DoubleParam corner_repel_speed("speed that repel will be kicking at in a corner", "STP/Action/repel", 6.0, 1.0, 10.0);
}

bool AI::HL::STP::Action::repel(const World &world, Player::Ptr player) {
	// bool kicked = false;
	const Field &f = world.field();
	const Point diff = world.ball().position() - player->position();

	// set to RAM_BALL instead of using chase
	if (!player->has_ball()) {
		Point dest = world.ball().position();
		if (dest.x < f.friendly_goal().x + Robot::MAX_RADIUS) { // avoid going inside the goal
			dest.x = f.friendly_goal().x + Robot::MAX_RADIUS;
		}
		ram(world, player, dest, diff.norm() * FAST);
		return false;
	}
	/*
	   // just shoot as long as it's not in backwards direction
	   if (player->orientation() < M_PI / 2 && player->orientation() > -M_PI / 2) {
	    player->autokick(10.0);
	    kicked = true;
	   }

	   player->move(world.ball().position(), diff.orientation(), diff.norm() * FAST);
	   player->prio(AI::Flags::MovePrio::HIGH);

	   return kicked;
	 */
	// all enemies are obstacles

	Evaluation::ShootData shoot_data = Evaluation::evaluate_shoot(world, player);
	if (!shoot_data.blocked) {
		return shoot_goal(world, player);
	}

	std::vector<Point> obstacles;
	const EnemyTeam &enemy = world.enemy_team();
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		obstacles.push_back(enemy.get(i)->position());
	}

	// vertical line at the enemy goal area
	// basically u want the ball to be somewhere there
	const Point p1(f.length() / 2.0, -f.width() / 2.0), p2(f.length() / 2.0, f.width() / 2.0);
	std::pair<Point, double> target = angle_sweep_circles(player->position(), p1, p2, obstacles, Robot::MAX_RADIUS);

	return shoot_target(world, player, target.first);
}

bool AI::HL::STP::Action::corner_repel(const World &world, Player::Ptr player) {
	const Field &f = world.field();
	const Point ball = world.ball().position();
	const Point diff = world.ball().position() - player->position();

	// if ball not in corner then just repel
	if (Predicates::ball_in_our_corner(world) || Predicates::ball_in_their_corner(world)) {
		return repel(world, player);
	}

	// set to RAM_BALL instead of using chase
	if (!player->has_ball()) {
		Point dest = world.ball().position();
		if (dest.x < f.friendly_goal().x + Robot::MAX_RADIUS) { // avoid going inside the goal
			dest.x = f.friendly_goal().x + Robot::MAX_RADIUS;
		}
		ram(world, player, dest, diff.norm() * FAST);
		return false;
	}

	Evaluation::ShootData shoot_data = Evaluation::evaluate_shoot(world, player);
	if (!shoot_data.blocked) {
		return shoot_goal(world, player);
	}

	std::vector<Point> obstacles;
	const EnemyTeam &enemy = world.enemy_team();
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		obstacles.push_back(enemy.get(i)->position());
	}

	// check circle in the middle and the centre line and find the best open spot to shoot at
	const Point p1(0.0, -f.centre_circle_radius()), p2(0.0, f.centre_circle_radius());
	std::pair<Point, double> centre_circle = angle_sweep_circles(player->position(), p1, p2, obstacles, Robot::MAX_RADIUS);

	const Point p3(0.0, -f.width() / 2.0), p4(0.0, f.width() / 2.0);
	std::pair<Point, double> centre_line = angle_sweep_circles(player->position(), p3, p4, obstacles, Robot::MAX_RADIUS);

	if (centre_circle.second > shoot_accuracy) {
		return shoot_target(world, player, centre_circle.first, corner_repel_speed);
	}
	return shoot_target(world, player, centre_line.first, corner_repel_speed);
}

