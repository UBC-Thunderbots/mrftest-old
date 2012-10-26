#include "ai/hl/stp/evaluation/ball.h"
#include "ai/util.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/param.h"

using namespace AI::HL::STP;

namespace {
	BoolParam calc_baller_always_return("Calc baller always return", "STP/ball", true);

	BoolParam smart_possess_ball("Smart possess ball (instead of has ball only)", "STP/ball", true);

	DoubleParam enemy_pivot_threshold("circle radius in front of enemy robot to consider possession (meters)", "STP/ball", 0.1, 0.0, 1.0);

	Player baller;

	std::vector<Robot> enemies;

	void update_enemies_by_grab_ball_dist(World world) {
		enemies = AI::HL::Util::get_robots(world.enemy_team());
		std::vector<double> score;

		for (unsigned i = 0; i < enemies.size(); ++i) {
			Point dest;
			AI::Util::calc_fastest_grab_ball_dest(world.ball().position(), world.ball().velocity(), enemies[i]->position(), dest);
			double dist = (enemies[i]->position() - dest).len();
			score.push_back(dist);
		}

		// BUBBLE SORT ROTFLOL
		for (unsigned i = 0; i < enemies.size(); ++i) {
			for (unsigned j = i + 1; j < enemies.size(); ++j) {
				if (score[i] > score[j]) {
					std::swap(enemies[i], enemies[j]);
					std::swap(score[i], score[j]);
				}
			}
		}
	}

	void update_baller(World world) {
		const FriendlyTeam friendly = world.friendly_team();
		// use has ball
		for (std::size_t i = 0; i < friendly.size(); ++i) {
			if (get_goalie() == friendly.get(i)) {
				continue;
			}
			if (friendly.get(i)->has_ball()) {
				baller = friendly.get(i);
				return;
			}
		}
		// use possess ball
		for (std::size_t i = 0; i < friendly.size(); ++i) {
			if (Evaluation::possess_ball(world, friendly.get(i))) {
				baller = friendly.get(i);
				return;
			}
		}

		if (!calc_baller_always_return) {
			baller = Player();
			return;
		}

		Player best;

		double min_dist = 1e99;
		for (std::size_t i = 0; i < friendly.size(); ++i) {
			Player player = friendly.get(i);
			Point dest = Evaluation::calc_fastest_grab_ball_dest(world, player);
			if (!best || min_dist > (dest - player->position()).len()) {
				min_dist = (dest - player->position()).len();
				best = player;
			}
		}

		baller = best;
	}
}

DoubleParam Evaluation::pivot_threshold("circle radius in front of robot to enable pivot (meters)", "STP/ball", 0.1, 0.0, 1.0);

bool Evaluation::ball_in_pivot_thresh(World world, Player player) {
	Point unit_vector = Point::of_angle(player->orientation());
	Point circle_center = player->position() + Robot::MAX_RADIUS * unit_vector;
	double dist = (circle_center - world.ball().position()).len();
	return dist < pivot_threshold;
}

bool Evaluation::possess_ball(World world, Player player) {
	if (player->has_ball()) {
		return true;
	}
	if (!smart_possess_ball) {
		return false;
	}
	return ball_in_pivot_thresh(world, player);
}

bool Evaluation::possess_ball(World world, Robot robot) {
	// true if in pivot thresh
	Point unit_vector = Point::of_angle(robot->orientation());
	Point circle_center = robot->position() + Robot::MAX_RADIUS * unit_vector;
	double dist = (circle_center - world.ball().position()).len();
	return dist < enemy_pivot_threshold;
}

Player Evaluation::calc_friendly_baller() {
	return baller;
}

Robot Evaluation::calc_enemy_baller(World world) {
	EnemyTeam enemy = world.enemy_team();
	Robot robot;
	double best_dist = 1e99;
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		if (possess_ball(world, enemy.get(i))) {
			return enemy.get(i);
		}
		Point dest;
		AI::Util::calc_fastest_grab_ball_dest(world.ball().position(), world.ball().velocity(), enemy.get(i)->position(), dest);
		double dist = (enemy.get(i)->position() - dest).len();
		if (!robot || dist < best_dist) {
			best_dist = dist;
			robot = enemy.get(i);
		}
	}
	return robot;
}

Point Evaluation::calc_fastest_grab_ball_dest(World world, Player player) {
	Point dest;
	AI::Util::calc_fastest_grab_ball_dest(world.ball().position(), world.ball().velocity(), player->position(), dest);
	return dest;
}

std::vector<Robot> Evaluation::enemies_by_grab_ball_dist() {
	return enemies;
}

void AI::HL::STP::Evaluation::tick_ball(World world) {
	update_baller(world);
	update_enemies_by_grab_ball_dist(world);
}

