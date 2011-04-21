#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/util.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace AI::HL::W;

namespace {
	const int GRID_X = 25;
	const int GRID_Y = 25;

	const double DEG_2_RAD = 1.0 / 180.0 * M_PI;

	// avoid enemy robots by at least this distance
	
	DoubleParam near_thresh("enemy avoidance distance (robot radius)", "STP/offense", 4.0, 1.0, 10.0);

	DoubleParam ball_dist_weight("ball distance weight", "STP/offense", 1.0, 0.0, 2.0);

	double passer_scoring_function(const World &world, const Point &passee_pos, const std::vector<Point> &enemy_pos, const Point &dest, const std::vector<Point> &dont_block) {
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
		std::pair<Point, double> bestshot = AI::HL::Util::calc_best_shot_target(passee_pos, enemy_pos, dest);
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
			std::pair<Point, double> shootershot = AI::HL::Util::calc_best_shot_target(passee_pos, enemy_pos, dont_block[i]);
			const Point diff1 = (shootershot.first - dont_block[i]);
			const Point diff2 = (dest - dont_block[i]);
			const double anglediff = angle_diff(diff1.orientation(), diff2.orientation());
			if (anglediff * 2 < shootershot.second) {
				return -1e99;
			}
		}

		// 10 degrees of shooting is 10 Points
		score *= 10.0 / (10.0 * DEG_2_RAD);

		/*

		// want to be as near to passee or ball as possible?
		const double ball_dist = (dest - world.ball().position()).len();
		const double passee_dist = (passee_pos - dest).len();
		
		// divide by largest distance?
		const double big_dist = std::max(ball_dist, passee_dist);
		score /= big_dist;
		*/
		//score *= closest_enemy;
		return score;
		
	}
	
	bool calc_passer_position_best(const World &world, const Point &passee_pos, const std::vector<Point> &enemy_pos, const std::vector<Point> &dont_block, Point &best_pos) {
		// divide up into grids
		const double x1 = -world.field().length() / 2;
		const double x2 = world.field().length() / 2;
		const double y1 = -world.field().width() / 2;
		const double y2 = world.field().width() / 2;

		const double dx = (x2 - x1) / (GRID_X + 1);
		const double dy = (y2 - y1) / (GRID_Y + 1);
		double best_score = -1e50;

		best_pos = Point();

		for (int i = 0; i < GRID_X; ++i) {
			for (int j = 0; j < GRID_Y; ++j) {
				const double x = x1 + dx * (i + 1);
				const double y = y1 + dy * (j + 1);
				const Point pos = Point(x, y);

				// TEMPORARY HACK!!
				// ensures that we do not get too close to the enemy defense area.
				const double goal_dist = (pos - world.field().enemy_goal()).len();
				if (goal_dist < world.field().goal_width()) {
					continue;
				}

				const double score = passer_scoring_function(world, passee_pos, enemy_pos, pos, dont_block);
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

	
	double passee_scoring_function(const World &world, const std::set<Player::CPtr> &players, const std::vector<Point> &enemy_pos, const Point &dest, const std::vector<Point> &dont_block) {
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

		// want to be as near to enemy goal as possible, can be close to ball too but that should be left for the passer in most cases
		const double ball_dist = (dest - world.ball().position()).len();
		const double goal_dist = (dest - bestshot.first).len();

		// weighted
		const double big_dist = ball_dist * 0.25 + goal_dist * 0.75;
		score /= big_dist;

		// divide by distance to nearest player
		double min_dist = 1e99;
		for (auto it = players.begin(); it != players.end(); ++it) {
			double dist = ((*it)->position() - dest).len();
			if (dist < min_dist) {
				min_dist = dist;
			}
		}

		score /= min_dist;
		//score *= closest_enemy;

		return score;
	}

	bool calc_passee_position_best(const World &world, const std::set<Player::CPtr> &players, const std::vector<Point> &enemy_pos, const std::vector<Point> &dont_block, Point &best_pos) {
		// divide up into grids
		const double x1 = -world.field().length() / 2;
		const double x2 = world.field().length() / 2;
		const double y1 = -world.field().width() / 2;
		const double y2 = world.field().width() / 2;

		const double dx = (x2 - x1) / (GRID_X + 1);
		const double dy = (y2 - y1) / (GRID_Y + 1);
		double best_score = -1e50;

		best_pos = Point();

		for (int i = 0; i < GRID_X; ++i) {
			for (int j = 0; j < GRID_Y; ++j) {
				const double x = x1 + dx * (i + 1);
				const double y = y1 + dy * (j + 1);
				const Point pos = Point(x, y);

				// TEMPORARY HACK!!
				// ensures that we do not get too close to the enemy defense area.
				const double goal_dist = (pos - world.field().enemy_goal()).len();
				if (goal_dist < world.field().goal_width()) {
					continue;
				}

				const double score = passee_scoring_function(world, players, enemy_pos, pos, dont_block);
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

bool AI::HL::STP::Evaluation::can_pass(const World& world, const Point pos) {
	return AI::HL::Util::path_check(world.ball().position(), pos, AI::HL::Util::get_robots(world.enemy_team()), Robot::MAX_RADIUS + Ball::RADIUS + AI::HL::Util::shoot_accuracy);
}

std::pair <Point,Point> AI::HL::STP::Evaluation::calc_pass_positions(const World &world, const std::set<Player::CPtr> &players) {

	std::pair <Point,Point> pp;
	pp.first = Point();
	pp.second = Point();

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
	const FriendlyTeam &friendly = world.friendly_team();
	for (size_t i = 0; i < friendly.size(); ++i) {
		if (players.find(friendly.get(i)) == players.end()) {
			dont_block.push_back(friendly.get(i)->position());
		}
	}

	// Maybe we should find the best combo of passee best and passer best, 
	// instead of just finding the best passee position and find the best passer best position relative to that passee position
	Point passee_best;
	if (!calc_passee_position_best(world, players, enemy_pos, dont_block, passee_best)) {
		LOG_WARN("could not find a good passee pos");
		return pp;
	} 
	pp.second = passee_best;
	Point passer_best;
	if (!calc_passer_position_best(world, passee_best, enemy_pos, dont_block, passer_best)){
		LOG_WARN("could not find a good passer pos");
		return pp;
	}
	pp.first = passer_best;
	return pp;
}
