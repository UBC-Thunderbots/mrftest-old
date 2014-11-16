#include "ai/hl/stp/evaluation/ball.h"
#include "ai/util.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/param.h"

using namespace AI::HL::STP;

namespace {
	BoolParam calc_baller_always_return(u8"Calc baller always return", u8"STP/ball", true);

	BoolParam smart_possess_ball(u8"Smart possess ball (instead of has ball only)", u8"STP/ball", true);

	DoubleParam enemy_pivot_threshold(u8"circle radius in front of enemy robot to consider possession (meters)", u8"STP/ball", 0.1, 0.0, 1.0);

	Player baller;

	std::vector<Robot> enemies;

	void update_enemies_by_grab_ball_dist(World world) {
		enemies = AI::HL::Util::get_robots(world.enemy_team());
		std::vector<double> score;

		for (Robot i : enemies) {
			Point dest;
			AI::Util::calc_fastest_grab_ball_dest(world.ball().position(), world.ball().velocity(), i.position(), dest);
			double dist = (i.position() - dest).len();
			score.push_back(dist);
		}

		unsigned int j;
		for (unsigned i = 1; i < enemies.size(); i++){
			j = i;
			while (j > 0 && score[j-1] > score[j]){
				std::swap(score[j-1], score[j]);
				std::swap(enemies[j-1], enemies[j]);
				j--;
			}
		}
	}

	void update_baller(World world) {
		const FriendlyTeam friendly = world.friendly_team();
		// use has ball
		for (const Player i : friendly) {
			if (get_goalie() == i) {
				continue;
			}
			if (i.has_ball()) {
				baller = i;
				return;
			}
		}
		// use possess ball
		for (const Player i : friendly) {
			if (Evaluation::possess_ball(world, i)) {
				baller = i;
				return;
			}
		}

		if (!calc_baller_always_return) {
			baller = Player();
			return;
		}

		Player best;

		double min_dist = 1e99;
		for (const Player i : friendly) {
			Point dest = Evaluation::calc_fastest_grab_ball_dest(world, i);
			if (!best || min_dist > (dest - i.position()).len()) {
				min_dist = (dest - i.position()).len();
				best = i;
			}
		}

		baller = best;
	}
}

DoubleParam Evaluation::pivot_threshold(u8"circle radius in front of robot to enable pivot (meters)", u8"STP/ball", 0.1, 0.0, 1.0);

bool Evaluation::ball_in_pivot_thresh(World world, Player player) {
	Point unit_vector = Point::of_angle(player.orientation());
	Point circle_center = player.position() + Robot::MAX_RADIUS * unit_vector;
	double dist = (circle_center - world.ball().position()).len();
	return dist < pivot_threshold;
}

bool Evaluation::possess_ball(World world, Player player) {
	if (player.has_ball()) {
		return true;
	}
	if (!smart_possess_ball) {
		return false;
	}
	return ball_in_pivot_thresh(world, player);
}

bool Evaluation::possess_ball(World world, Robot robot) {
	// true if in pivot thresh
	Point unit_vector = Point::of_angle(robot.orientation());
	Point circle_center = robot.position() + Robot::MAX_RADIUS * unit_vector;
	double dist = (circle_center - world.ball().position()).len();
	return dist < enemy_pivot_threshold;
}

Player Evaluation::calc_friendly_baller() {
	return baller;
}

Robot Evaluation::calc_enemy_baller(World world) {
	Robot robot;
	double best_dist = 1e99;
	for (const Robot i : world.enemy_team()) {
		if (possess_ball(world, i)) {
			return i;
		}
		Point dest;
		AI::Util::calc_fastest_grab_ball_dest(world.ball().position(), world.ball().velocity(), i.position(), dest);
		double dist = (i.position() - dest).len();
		if (!robot || dist < best_dist) {
			best_dist = dist;
			robot = i;
		}
	}
	return robot;
}

Point Evaluation::calc_fastest_grab_ball_dest(World world, Player player) {
	Point dest;
	AI::Util::calc_fastest_grab_ball_dest(world.ball().position(), world.ball().velocity(), player.position(), dest);
	return dest;
}

std::vector<Robot> Evaluation::enemies_by_grab_ball_dist() {
	return enemies;
}

void AI::HL::STP::Evaluation::tick_ball(World world) {
	update_baller(world);
	update_enemies_by_grab_ball_dist(world);
}

