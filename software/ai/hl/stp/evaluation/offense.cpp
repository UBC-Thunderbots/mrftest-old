#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/util.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "uicomponents/param.h"
#include "util/dprint.h"

using namespace AI::HL::W;
//using AI::HL::STP::Evaluation::OffenseData;
//using AI::HL::STP::Evaluation::EvaluateOffense;

namespace {
	const int GRID_X = 25;
	const int GRID_Y = 25;

	const double DEG_2_RAD = 1.0 / 180.0 * M_PI;

	// avoid enemy robots by at least this distance
	const double NEAR = Robot::MAX_RADIUS * 3;

	double scoring_function(World& world, const std::set<Player::Ptr>& players, const std::vector<Point>& enemy_pos, const Point& dest, const std::vector<Point>& dont_block) {
		// can't be too close to enemy
		for (std::size_t i = 0; i < enemy_pos.size(); ++i) {
			if ((enemy_pos[i] - dest).len() < NEAR) {
				return -1e99;
			}
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
			if (diff2.len() < NEAR) {
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
		const double balldist = (dest - world.ball().position()).len();
		const double goal_dist = (dest - bestshot.first).len();

		// divide by largest distance?
		const double bigdist = std::max(balldist, goal_dist);
		score /= bigdist;

		// divide by distance to nearest player
		double mindist = 1e99;
		for (auto it = players.begin(); it != players.end(); ++it) {
			double dist = ((*it)->position() - dest).len();
			if (dist < mindist) {
				mindist = dist;
			}
		}

		score /= mindist;

		return score;
	}

	bool calc_position_best(World& world, const std::set<Player::Ptr>& players, const std::vector<Point> &enemy_pos, const std::vector<Point> &dont_block, Point &best_pos) {
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

				const double score = scoring_function(world, players, enemy_pos, pos, dont_block);
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

	Point calc_positions(World& world, const std::set<Player::Ptr>& players) {

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
		FriendlyTeam& friendly = world.friendly_team();
		for (size_t i = 0; i < friendly.size(); ++i) {
			if (players.find(friendly.get(i)) == players.end()) {
				dont_block.push_back(friendly.get(i)->position());
			}
		}

		Point best;
		if (!calc_position_best(world, players, enemy_pos, dont_block, best)) {
			LOG_WARN("could not find a good plac3");
			return Point();
		}
		return best;
	}
}

Point AI::HL::STP::Evaluation::evaluate_offense(AI::HL::W::World& world, const std::set<Player::Ptr>& players) {
	return calc_positions(world, players);
}

