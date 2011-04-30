#include "ai/hl/stp/action/repel.h"
#include "ai/flags.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace AI::HL::STP;

namespace {
	const double FAST = 100.0;
}

void AI::HL::STP::Action::repel(const World &world, Player::Ptr player) {
	const Point diff = world.ball().position() - player->position();

	// set to RAM_BALL instead of using chase
	if (!player->has_ball()) {
		Point des = world.ball().position();
		if (des.x < -world.field().length()/2+0.2) { // avoid going inside the goal
			des.x = -world.field().length()/2+0.2;
		}
		player->move(des, diff.orientation(), diff.norm() * FAST);
		player->type(AI::Flags::MoveType::RAM_BALL);
		player->prio(AI::Flags::MovePrio::HIGH);
		return;
	}

	// just shoot as long as it's not in backwards direction
	if (player->orientation() < M_PI / 2 && player->orientation() > -M_PI / 2) {
		 player->autokick(10.0);
	}

	player->move(world.ball().position(), diff.orientation(), diff.norm() * FAST);
	player->prio(AI::Flags::MovePrio::HIGH);

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

	AI::HL::STP::Action::shoot(world, player, flags, target.first);
	 */
}

