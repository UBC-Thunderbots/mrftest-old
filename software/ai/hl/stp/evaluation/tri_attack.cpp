#include "ai/hl/stp/evaluation/tri_attack.h"
#include "ai/hl/stp/evaluation/enemy.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/ball_threat.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "util/algorithm.h"
#include "util/dprint.h"
#include "util/param.h"
#include "geom/angle.h"

#include <vector>
#include <cmath>

using namespace AI::HL::W;
using namespace AI::HL::STP::Evaluation;

#define GRID_X 20.0
#define GRID_Y 15.0

Point AI::HL::STP::Evaluation::tri_attack_evaluation(World world) {

	int enemy_free_zone_importance = 50;
	int shooting_distance_importance = 30;
	int shooting_angle_importance = 20;
	double danger_zone = 0.4, distance, danger_level, less_danger_position = 100000, angle;
	//size_t enemy_team_size = world.enemy_team().size();
	std::vector<Robot> enemies = AI::HL::Util::get_robots(world.enemy_team());
//			Evaluation::enemies_by_grab_ball_dist();
	int index;
	Point point_underevaluation = Point(0,0);
	Point best_point = Point(0,0);
	int robot_count;

	for (double i = 1; i <= GRID_X; i++ ) {
		for (double j = 1; j <= GRID_Y; j++) {
			index = 0;
			robot_count = 0;
			double x = world.field().width() * (i/GRID_X);
			double y = world.field().width() * (j/GRID_Y);
			
			point_underevaluation.x = x;
			point_underevaluation.y = y;
			do {
				// Calculate how many enemies are within the danger_zone
				distance = (point_underevaluation - enemies[index].position()).len();
				if(distance < danger_zone) { robot_count++; index++;}			
			} while (distance < danger_zone);

			// Calculate distance from net
			distance = (point_underevaluation - world.field().enemy_goal()).len();

			// Calculate shooting angle
			double ang = fabs((x-2.0)/y);
			angle = tan(ang);
			danger_level = enemy_free_zone_importance*(robot_count/6) + shooting_distance_importance*(distance/5.5) + angle*shooting_angle_importance;

			if(danger_level < less_danger_position) {
				less_danger_position = danger_level;
				best_point = point_underevaluation;
			}
		}
	}


	return best_point;
}
