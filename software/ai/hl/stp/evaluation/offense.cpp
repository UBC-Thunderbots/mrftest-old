#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/util.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "util/dprint.h"

using namespace AI::HL::STP;

// using AI::HL::STP::Evaluation::OffenseData;
// using AI::HL::STP::Evaluation::EvaluateOffense;

IntParam AI::HL::STP::Evaluation::grid_x("grid x size", "STP/offense", 25, 1, 100);
IntParam AI::HL::STP::Evaluation::grid_y("grid y size", "STP/offense", 25, 1, 100);

using AI::HL::STP::Evaluation::grid_x;
using AI::HL::STP::Evaluation::grid_y;

namespace {
	//const double DEG_2_RAD = 1.0 / 180.0 * M_PI;

	// BoolParam use_area_metric("use area metric", "STP/offense", true);

	DoubleParam near_thresh("enemy avoidance distance (robot radius)", "STP/offense", 4.0, 1.0, 10.0);

	DoubleParam weight_total("Scoring weight for everything", "STP/offense", 1.0, 0.0, 99999999.0);

	DoubleParam weight_goal_angle("Scoring weight for angle to goal (POWER, careful)", "STP/offense", 1.0, 0.0, 99.0);

	DoubleParam weight_progress("Scoring weight for ball progress (+ve)", "STP/offense", 1.0, 0.0, 99.0);

	DoubleParam weight_ball_angle("Scoring weight for angle from ball to goal at robot (-ve)", "STP/offense", 1.0, 0.0, 99.0);

	DoubleParam weight_ball_dist("Scoring weight for distance to ball (-ve)", "STP/offense", 1.0, 0.0, 99.0);

	DoubleParam weight_enemy("Scoring weight for nearest enemy (+ve)", "STP/offense", 1.0, 0.0, 99.0);

	DoubleParam weight_goal_dist("Scoring weight for distance to enemy goal (-ve)", "STP/offense", 1.0, 0.0, 99.0);

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
		const double score_enemy = closest_enemy;

		double closest_friendly = 1e99;
		const FriendlyTeam &friendly = world.friendly_team();
		for (size_t i = 0; i < friendly.size(); ++i) {
			double dist = (friendly.get(i)->position() - dest).len();
			closest_friendly = std::min(closest_friendly, dist);
		}

		if (closest_friendly > closest_enemy + Robot::MAX_RADIUS) {
			return -1e99;
		}

		double score_progress = (dest - world.ball().position()).x;

		double score_goal_angle = AI::HL::Util::calc_best_shot(world.field(), enemy_pos, dest).second;

		if (score_goal_angle < degrees2radians(min_shoot_region)) {
			return -1e99;
		}

		// TODO: check the line below here
		// scoring factors:
		// density of enemy, passing distance, distance to the goal, angle of shooting, angle of receiving
		// distance toward the closest enemy, travel distance, behind of in front of the enemy

#warning using deprecated method
		if (!AI::HL::Util::path_check(world.ball().position(), dest, enemy_pos, Robot::MAX_RADIUS + Ball::RADIUS * 2)) {
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

		// want to be as near to enemy goal or ball as possible
		// const double ball_dist = (dest - world.ball().position()).len();
		// const double goal_dist = (dest - bestshot.first).len();

		double d1 = (world.ball().position() - dest).orientation();
		double d2 = (world.field().enemy_goal() - dest).orientation();
		const double score_ball_angle = angle_diff(d1, d2);

		const double score_ball_dist = (world.ball().position() - dest).len();

		const double score_goal_dist = (world.field().enemy_goal() - dest).len();

		// const double raw_score = weight_goal * score_goal - weight_ball_angle * score_ball_angle - weight_ball_dist * score_ball_dist + weight_enemy * score_enemy;

		// how "heavy" do u want the goal angle to be
		double raw_score = pow(score_goal_angle, weight_goal_angle);

		// want further from enemy
		raw_score *= (1 + weight_enemy * score_enemy);

		// the nearer the distance, the closer to 1
		raw_score /= (1 + weight_ball_dist * score_ball_dist);

		// the smaller the angle, the closer to 1
		raw_score /= (1 + weight_ball_angle * score_ball_angle);

		// want more progress
		raw_score *= (1 + weight_progress * score_progress);

		// want more distance from enemy goal
		raw_score *= (1 + weight_goal_dist * score_goal_dist);

		return weight_total * raw_score;
	}

	bool calc_position_best(const World &world, const std::vector<Point> &enemy_pos, const std::vector<Point> &dont_block, Point &best_pos) {
		// divide up into a hexagonal grid
		const double x1 = -world.field().length() / 2, x2 = -x1;
		const double y1 = -world.field().width() / 2, y2 = -y1;

		// for the spacing to be uniform, we need dy = sqrt(3/4)*dx
		const double dx = (x2 - x1) / (grid_x + 1) / 2;
		const double dy = (y2 - y1) / (grid_y + 1) / 2;
		double best_score = -1e50;

		best_pos = Point();

		std::vector<std::vector<double> > scores;

		/*
		   if (use_area_metric) {
		   scores.resize(2*grid_y+1);
		   for (int i = 0; i < 2*grid_y+1; ++i) {
		   scores[i].resize(2*grid_x+1, 0);
		   }
		   }
		 */

		for (int i = 1; i <= 2*grid_y+1; i += 2) {
			for (int j = i%2+1; j <= 2*grid_x+1; j += 2) {
				const double x = x1 + dx * j;
				const double y = y1 + dy * i;
				const Point pos = Point(x, y);

				// TEMPORARY HACK!!
				// ensures that we do not get too close to the enemy defense area.
				const double goal_dist = (pos - world.field().enemy_goal()).len();
				if (goal_dist < world.field().goal_width()) {
					continue;
				}

				double score = scoring_function(world, enemy_pos, pos, dont_block);

				if (score > best_score) {
					best_score = score;
					best_pos = pos;
				}

				/*
				   if (use_area_metric) {
				   scores[i][j] = score;
				   }
				 */
			}
		}

		/*
		   if (use_area_metric) {

		   }
		 */

		return best_score > 0;
	}

	// TODO: explore updating the offensive function only ONCE
	/*
	   std::vector<std::vector<double> > scores;

	   void update() {
	   scores.clear();
	   scores.resize(2*grid_y+1);
	   for (int i = 0; i < 2*grid_y+1; ++i) {
	   scores[i].resize(2*grid_x+1, 0);
	   }
	   }
	 */
}

double AI::HL::STP::Evaluation::offense_score(const World &world, const Point dest) {
	const EnemyTeam &enemy = world.enemy_team();

	std::vector<Point> enemy_pos;
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		enemy_pos.push_back(enemy.get(i)->position());
	}

	std::vector<Point> dont_block;
	dont_block.push_back(world.ball().position());

	return scoring_function(world, enemy_pos, dest, dont_block);
}

std::array<Point, 2> AI::HL::STP::Evaluation::offense_positions(const World &world) {
	// just for caching..
	const EnemyTeam &enemy = world.enemy_team();
	std::vector<Point> enemy_pos;
	for (size_t i = 0; i < enemy.size(); ++i) {
		enemy_pos.push_back(enemy.get(i)->position());
	}

	// TODO: optimize using the matrix below
	// std::vector<std::vector<bool>> grid(GRID_X, std::vector<bool>(GRID_Y, true));

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

Point AI::HL::STP::Evaluation::passee_position(const World &world) {

	const EnemyTeam &enemy = world.enemy_team();
	std::vector<Point> enemy_pos;
	for (size_t i = 0; i < enemy.size(); ++i) {
		enemy_pos.push_back(enemy.get(i)->position());
	}

	std::vector<Point> dont_block;
	dont_block.push_back(world.ball().position());

	Point best;
	calc_position_best(world, enemy_pos, dont_block, best);

	return best;
}

