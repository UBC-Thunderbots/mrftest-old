#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/util.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "util/dprint.h"

using namespace AI::HL::W;
// using AI::HL::STP::Evaluation::OffenseData;
// using AI::HL::STP::Evaluation::EvaluateOffense;

IntParam AI::HL::STP::Evaluation::grid_x("grid x size", "STP/offense", 25, 1, 100);
IntParam AI::HL::STP::Evaluation::grid_y("grid y size", "STP/offense", 25, 1, 100);

using AI::HL::STP::Evaluation::grid_x;
using AI::HL::STP::Evaluation::grid_y;

namespace {

	const double DEG_2_RAD = 1.0 / 180.0 * M_PI;

	DoubleParam near_thresh("enemy avoidance distance (robot radius)", "STP/offense", 4.0, 1.0, 10.0);

	DoubleParam ball_dist_weight("ball distance weight", "STP/offense", 1.0, 0.0, 2.0);

	double scoring_function(const World &world, const std::vector<Point> &enemy_pos, const Point &dest, const std::vector<Point> &dont_block) {

		// can't be too close to enemy
		double closest_enemy = world.field().width();
		for (std::size_t i = 0; i < enemy_pos.size(); ++i) {
			double dist = (enemy_pos[i] - dest).len();
			if (dist < near_thresh * Robot::MAX_RADIUS) {
				return -1e99;
			}
			closest_enemy = std::min(closest_enemy, dist);
		}

		// Hmm.. not sure if having negative number is a good idea.
		std::pair<Point, double> bestshot = AI::HL::Util::calc_best_shot(world.field(), enemy_pos, dest);
		double score = bestshot.second;

		// TODO: check the line below here
		// scoring factors:
		// density of enemy, passing distance, distance to the goal, angle of shooting, angle of receiving
		// distance toward the closest enemy, travel distance, behind of in front of the enemy

		// TODO: fix this
		if (!AI::HL::Util::path_check(world.ball().position(), dest, enemy_pos, Robot::MAX_RADIUS + Ball::RADIUS * 3)) {
			return -1e99;
		}

		for (size_t i = 0; i < dont_block.size(); ++i) {
			const Point diff2 = (dest - dont_block[i]);
			if (diff2.len() < near_thresh * Robot::MAX_RADIUS) {
				return -1e99;
			}
		}

		// super expensive calculation
		// basically, ensures that this position does not block the list of positions
		// inside dont_block from view of goal.
		for (size_t i = 0; i < dont_block.size(); ++i) {
			std::pair<Point, double> shootershot = AI::HL::Util::calc_best_shot(world.field(), enemy_pos, dont_block[i]);
			const Point diff1 = (shootershot.first - dont_block[i]);
			const Point diff2 = (dest - dont_block[i]);
			const double anglediff = angle_diff(diff1.orientation(), diff2.orientation());
			if (anglediff * 2 < shootershot.second) {
				return -1e99;
			}
		}

		// 10 degrees of shooting is 10 Points
		score *= 10.0 / (10.0 * DEG_2_RAD);

		// want to be as near to enemy goal or ball as possible
		const double ball_dist = (dest - world.ball().position()).len() + ball_dist_weight;
		// const double goal_dist = (dest - bestshot.first).len();

		// divide by largest distance?
		//const double bigdist = std::max(ball_dist, goal_dist);
		//score /= bigdist;

		score /= ball_dist;

		// score *= closest_enemy;

		return score;
	}

	bool calc_position_best(const World &world, const std::vector<Point> &enemy_pos, const std::vector<Point> &dont_block, Point &best_pos) {
		// divide up into grids
		const double x1 = -world.field().length() / 2;
		const double x2 = world.field().length() / 2;
		const double y1 = -world.field().width() / 2;
		const double y2 = world.field().width() / 2;

		const double dx = (x2 - x1) / (grid_x + 1);
		const double dy = (y2 - y1) / (grid_y + 1);
		double best_score = -1e50;

		best_pos = Point();

		for (int i = 0; i < grid_x; ++i) {
			for (int j = 0; j < grid_y; ++j) {
				const double x = x1 + dx * (i + 1);
				const double y = y1 + dy * (j + 1);
				const Point pos = Point(x, y);

				// TEMPORARY HACK!!
				// ensures that we do not get too close to the enemy defense area.
				const double goal_dist = (pos - world.field().enemy_goal()).len();
				if (goal_dist < world.field().goal_width()) {
					continue;
				}

				const double score = scoring_function(world, enemy_pos, pos, dont_block);
				if (score < -1e50) {
					continue;
				}
				if (score > best_score) {
					best_score = score;
					best_pos = pos;
				}
			}
		}

		return best_score > -1e40;
	}

}

double AI::HL::STP::Evaluation::offense_score(const World &world, const Point dest) {

	const EnemyTeam& enemy = world.enemy_team();

	std::vector<Point> enemy_pos;
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		enemy_pos.push_back(enemy.get(i)->position());
	}

	std::vector<Point> dont_block;
	dont_block.push_back(world.ball().position());

	return scoring_function(world, enemy_pos, dest, dont_block);
}

std::array<Point, 2> AI::HL::STP::Evaluation::offense_positions(const World& world) {
	// just for caching..
	const EnemyTeam &enemy = world.enemy_team();
	std::vector<Point> enemy_pos;
	for (size_t i = 0; i < enemy.size(); ++i) {
		enemy_pos.push_back(enemy.get(i)->position());
	}

	// TODO: optimize using the matrix below
	// std::vector<std::vector<bool> > grid(GRID_X, std::vector<bool>(GRID_Y, true));

	// don't block ball, and the others
	std::vector<Point> dont_block;
	dont_block.push_back(world.ball().position());
	/*
	   const FriendlyTeam &friendly = world.friendly_team();
	   for (size_t i = 0; i < friendly.size(); ++i) {
	   if (players.find(friendly.get(i)) == players.end()) {
	   dont_block.push_back(friendly.get(i)->position());
	   }
	   }
	   */

	std::array<Point, 2> best;

	calc_position_best(world, enemy_pos, dont_block, best[0]);

	dont_block.push_back(best[0]);
	calc_position_best(world, enemy_pos, dont_block, best[1]);

	return best;
}

