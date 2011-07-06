#include "ai/hl/stp/action/repel.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/flags.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "util/param.h"
#include "ai/hl/stp/evaluation/shoot.h"

using namespace AI::HL::STP;

namespace {
	const double FAST = 100.0;
}

bool AI::HL::STP::Action::repel(const World &world, Player::Ptr player) {
	
	//bool kicked = false;
	const Point diff = world.ball().position() - player->position();

	// set to RAM_BALL instead of using chase
	if (!player->has_ball()) {
		Point dest = world.ball().position();
		if (dest.x < world.field().friendly_goal().x + Robot::MAX_RADIUS) { // avoid going inside the goal
			dest.x = world.field().friendly_goal().x + Robot::MAX_RADIUS;
		}
		player->move(dest, diff.orientation(), diff.norm() * FAST);
		player->type(AI::Flags::MoveType::RAM_BALL); 
		player->prio(AI::Flags::MovePrio::HIGH);
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
	if (!shoot_data.blocked) { // still blocked, just aim
		return shoot_goal(world, player);
	}
	
	std::vector<Point> obstacles;
	const EnemyTeam &enemy = world.enemy_team();
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		obstacles.push_back(enemy.get(i)->position());
	}

	const Field &f = world.field();

	// vertical line at the enemy goal area
	// basically u want the ball to be somewhere there
	const Point p1(f.length() / 2.0, -f.width() / 2.0);
	const Point p2(f.length() / 2.0, f.width() / 2.0);
	std::pair<Point, double> target = angle_sweep_circles(player->position(), p1, p2, obstacles, Robot::MAX_RADIUS);

	return shoot_target(world, player, target.first);
	 
}

bool AI::HL::STP::Action::corner_repel(const World &world, Player::Ptr player) {
	
	const Point diff = world.ball().position() - player->position();

	// set to RAM_BALL instead of using chase
	if (!player->has_ball()) {
		Point dest = world.ball().position();
		if (dest.x < world.field().friendly_goal().x + Robot::MAX_RADIUS) { // avoid going inside the goal
			dest.x = world.field().friendly_goal().x + Robot::MAX_RADIUS;
		}
		player->move(dest, diff.orientation(), diff.norm() * FAST);
		player->type(AI::Flags::MoveType::RAM_BALL); 
		player->prio(AI::Flags::MovePrio::HIGH);
		return false;
	}
	
	Evaluation::ShootData shoot_data = Evaluation::evaluate_shoot(world, player);
	if (!shoot_data.blocked) { // still blocked, just aim
		return shoot_goal(world, player);
	}
	
	std::vector<Point> obstacles;
	const EnemyTeam &enemy = world.enemy_team();
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		obstacles.push_back(enemy.get(i)->position());
	}

	const Field &f = world.field();

	// vertical line at the enemy goal area
	// basically u want the ball to be somewhere there
	
	const Point p1(0.0, -f.centre_circle_radius());
	const Point p2(0.0, f.centre_circle_radius());
	std::pair<Point, double> centre_circle = angle_sweep_circles(player->position(), p1, p2, obstacles, Robot::MAX_RADIUS);
	
	const Point p3(0.0, -f.width() / 2.0);
	const Point p4(0.0, f.width() / 2.0);
	std::pair<Point, double> centre_line = angle_sweep_circles(player->position(), p3, p4, obstacles, Robot::MAX_RADIUS);
	
	if (centre_circle.second > centre_line.second) return shoot_target(world, player, centre_circle.first);
	return shoot_target(world, player, centre_line.first);
	 
}


