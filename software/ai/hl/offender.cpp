#include "ai/hl/offender.h"
#include "ai/hl/tactics.h"
#include "ai/hl/util.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "util/dprint.h"

#include "uicomponents/param.h"

#include <iostream>

using namespace AI::HL::W;

namespace {
	IntParam grid_x_size("Grid X size", 25, 1, 100);
	IntParam grid_y_size("Grid Y size", 25, 1, 100);

	const double DEG_2_RAD = 1.0 / 180.0 * M_PI;

	// avoid enemy robots by at least this distance
	const double NEAR = Robot::MAX_RADIUS * 3;
};

AI::HL::Offender::Offender(World &w) : world(w), chase(true) {
	grid.resize(grid_x_size, std::vector<bool>(grid_y_size));
}

double AI::HL::Offender::scoring_function(const std::vector<Point> &enemy_pos, const Point &pos, const std::vector<Point> &dont_block) const {
	// can't be too close to enemy
	for (std::size_t i = 0; i < enemy_pos.size(); ++i) {
		if ((enemy_pos[i] - pos).len() < NEAR) {
			return -1e99;
		}
	}

	// Hmm.. not sure if having negative number is a good idea.
	std::pair<Point, double> bestshot = AI::HL::Util::calc_best_shot(world.field(), enemy_pos, pos);
	double score = bestshot.second;

	// TODO: check the line below here
	// scoring factors:
	// density of enemy, passing distance, distance to the goal, angle of shooting, angle of receiving
	// distance toward the closest enemy, travel distance, behind of in front of the enemy

	// TODO: fix this
	if (!AI::HL::Util::path_check(world.ball().position(), pos, enemy_pos, Robot::MAX_RADIUS + Ball::RADIUS * 3)) {
		// return -1e99;
	}

	for (size_t i = 0; i < dont_block.size(); ++i) {
		const Point diff2 = (pos - dont_block[i]);
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
		const Point diff2 = (pos - dont_block[i]);
		const double anglediff = angle_diff(diff1.orientation(), diff2.orientation());
		if (anglediff * 2 < shootershot.second) {
			return -1e99;
		}
	}

	// 10 degrees of shooting is 10 Points
	score *= 10.0 / (10.0 * DEG_2_RAD);

	// want to be as near to our own goal as possible
	// score -= 1.0 * pos.x;
	const double balldist = (pos - world.ball().position()).len();
	const double goal_dist = (pos - bestshot.first).len();

	// divide by largest distance?
	const double bigdist = std::max(balldist, goal_dist);
	score /= bigdist;

	return score;
}

bool AI::HL::Offender::calc_position_best(const std::vector<Point> &enemy_pos, const std::vector<Point> &dont_block, Point &best_pos) {
	// divide up into grids
	const double x1 = -world.field().length() / 2;
	const double x2 = world.field().length() / 2;
	const double y1 = -world.field().width() / 2;
	const double y2 = world.field().width() / 2;

	const double dx = (x2 - x1) / (grid_x_size + 1);
	const double dy = (y2 - y1) / (grid_y_size + 1);
	double best_score = -1e50;

	best_pos = Point(0, 0);
	for (int i = 0; i < grid_x_size; ++i) {
		for (int j = 0; j < grid_y_size; ++j) {
			if (!grid[i][j]) {
				continue;
			}
			const double x = x1 + dx * (i + 1);
			const double y = y1 + dy * (j + 1);
			const Point pos = Point(x, y);

			// TEMPORARY HACK!!
			// ensures that we do not get too close to the enemy defense area.
			const double goal_dist = (pos - world.field().enemy_goal()).len();
			if (goal_dist < world.field().goal_width()) {
				grid[i][j] = false;
				continue;
			}

			const double score = scoring_function(enemy_pos, pos, dont_block);
			if (score < -1e50) {
				grid[i][j] = false;
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

std::vector<Point> AI::HL::Offender::calc_positions(const unsigned int n) {
	EnemyTeam &enemy = world.enemy_team();
	std::vector<Point> enemy_pos;
	for (size_t i = 0; i < enemy.size(); ++i) {
		enemy_pos.push_back(enemy.get(i)->position());
	}

	// TODO: optimize using the matrix below
	for (int i = 0; i < grid_x_size; ++i) {
		for (int j = 0; j < grid_y_size; ++j) {
			grid[i][j] = true;
		}
	}

	std::vector<Point> dont_block;
	dont_block.push_back(world.ball().position());
	std::vector<Point> positions;
	for (size_t i = 0; i < n; ++i) {
		Point best;
		if (!calc_position_best(enemy_pos, dont_block, best)) {
			break;
		}
		positions.push_back(best);
		dont_block.push_back(best);
	}
	return positions;
}

void AI::HL::Offender::tick() {
	if (players.size() == 0) {
		return;
	}

	// Sort by distance to ball. DO NOT SORT AGAIN!!
	std::sort(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(world.ball().position()));

	Player::Ptr chaser;
	if (chase) {
		// one player to chase the ball.
		chaser = players[0];
		// TODO: print out name?
	} else {
		for (std::size_t i = 0; i < players.size(); ++i) {
			if (players[i]->has_ball()) {
				chaser = players[i];
				break;
			}
		}
	}

	std::vector<Player::Ptr> supporters;
	std::vector<Point> locations;
	for (std::size_t i = 0; i < players.size(); ++i) {
		if (players[i] == chaser) {
			continue;
		}
		supporters.push_back(players[i]);
		locations.push_back(players[i]->position());
	}

	// calculate positions
	std::vector<Point> waypoints = calc_positions(static_cast<int>(supporters.size()));
	std::vector<std::size_t> order = dist_matching(locations, waypoints);

	// now do movement
	unsigned int flags = AI::Flags::calc_flags(world.playtype());

	{
		std::size_t w = 0; // so we can skip robots as needed
		for (std::size_t i = 0; i < supporters.size(); ++i) {
			if (w >= waypoints.size()) {
				// LOG_WARN(Glib::ustring::compose("%1 nothing to do", supporters[i]->name));
				continue;
			}

			supporters[i]->move(waypoints[order[w]], (world.ball().position() - supporters[i]->position()).orientation(), flags, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
			++w;
		}
	}

	// TODO: player with the ball to pass or to shoot

	// Idea for checking whether to pass or not:
	// check for enemy robots in a certain threshold distance,
	// if # of enemy robots within a certain distance is > 2 then you better try to pass before you get cornered
	// but if your supporters can't receive you better just shoot.
	/*
	   std::vector<Robot::Ptr> enemies = AI::HL::Util::get_robots(world.enemy_team());

	   double threshold_dist = 5 * Robot::MAX_RADIUS; // adjust
	   int cnt = 0;
	   bool pass = false;
	   for (std::size_t i = 0; i < enemies.size() ; ++i) {
	    dist = (chaser->position() - enemies[i]->position()).len();
	        if (dist <= threshold_dist) cnt++;
	   }

	   pass = (cnt >= 2) && AI::HL::Util::choose_best_pass(world, supporters) >= 0;

	 */
	Player::Ptr passee;
	if (AI::HL::Util::choose_best_pass(world, supporters) >= 0) {
		passee = supporters[AI::HL::Util::choose_best_pass(world, supporters)];
	}


	if (chaser.is()) {
		// TODO: do something more sensible
		if (AI::HL::Util::calc_best_shot(world, chaser, Robot::MAX_RADIUS).second <= 10) {
			AI::HL::Tactics::shoot(world, chaser, flags);
		} else if (passee.is()) {
			AI::HL::Tactics::pass(world, chaser, passee, flags);
		} else {
			AI::HL::Tactics::shoot(world, chaser, flags);
		}
	}
}

