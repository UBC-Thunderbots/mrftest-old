#include "indirect_chip.h"
#include "ai/hl/world.h"
#include "ai/hl/util.h"
#include "geom/point.h"
#include "geom/util.h"
#include "ai/hl/util.h"
#include "ai/common/field.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "geom/util.h"
#include <math.h>
#include "util/dprint.h"
#include "util/param.h"

using namespace std;
using namespace AI::HL::W;
using namespace AI::HL::STP;
using namespace AI::HL::Util;
using namespace AI::HL::STP::Evaluation;
using namespace Geom;


namespace {
	DoubleParam CHIP_TARGET_FRACTION(u8"chip_dist_fraction, adjusts how far between ball and net player will try chip", u8"AI/HL/STP/Tactic/indirect_chip", 5.0/10.0, 0.0, 100.0);
	DoubleParam CHIP_POWER_BOUNCE_THRESHOLD(u8"chip dist bounce threshold. adjusts how far from the kicker the first bounce should be so the ball will not bounce into the goal", u8"AI/HL/STP/Tactic/indirect_chip", 6.5/10.0, 0.0, 100.0);
	DoubleParam MAX_CHIP_POWER(u8"max power the robot can chip the ball at without problems", u8"AI/HL/STP/Tactic/indirect_chip", 2.0, 0.0, 100.0);
}


Point AI::HL::STP::Evaluation::indirect_chip_target(World world, Player player) {
	//use angle sweep? point is correcnt len?
	/* When passing to angle_sweep through get best shot, does return value changed based on what side of the field we're on
	 * since it passes different goalposts first depending on side of field. Order matters in angle sweep?
	 */

	Point enemy_goal_positive = world.field().enemy_goal_boundary().first.x > 0.0 ?
		world.field().enemy_goal_boundary().first : world.field().enemy_goal_boundary().second;
	Point enemy_goal_negative = world.field().enemy_goal_boundary().first.x < 0.0 ?
		world.field().enemy_goal_boundary().first : world.field().enemy_goal_boundary().second;

	std::vector<Point> blocking_enemies;
	Triangle chip_target_area = triangle(world.ball().position(), enemy_goal_positive, enemy_goal_negative);
	double shortest_blocker_dist = ((world.field().enemy_goal() - world.ball().position()).len());

	for (auto i : world.enemy_team()) {
		if(contains(chip_target_area, i.position()) || offset_to_line(world.ball().position(), enemy_goal_negative, i.position()) < Robot::MAX_RADIUS ||
				offset_to_line(world.ball().position(), enemy_goal_positive, i.position()) < Robot::MAX_RADIUS) {
			blocking_enemies.push_back(i.position());

			if((i.position() - world.ball().position()).len() < shortest_blocker_dist) {
				shortest_blocker_dist = (i.position() - world.ball().position()).len();//doens't account for robot radius
			}
		}
	}

	std::vector<Point> far_blocking_enemies;
	for (unsigned i = 0; blocking_enemies.size(); i++) {
		if((blocking_enemies[i] - world.ball().position()).len() > shortest_blocker_dist + Robot::MAX_RADIUS * 3.0) {
			far_blocking_enemies.push_back(blocking_enemies[i]);
		}
	}

	//should not consider first blocker
	Point chip_net_target = angle_sweep_circles(world.ball().position(), enemy_goal_negative, enemy_goal_positive, far_blocking_enemies, Robot::MAX_RADIUS).first;
	Point chip_dir = chip_net_target - world.ball().position();//from ball to point in net
	double total_chip_dist = chip_dir.len();
	Point max_target = world.ball().position() + chip_dir.norm(total_chip_dist * CHIP_POWER_BOUNCE_THRESHOLD);//on line from ball to net
	Point target = world.ball().position() + chip_dir.norm(total_chip_dist * CHIP_TARGET_FRACTION);

	//has valid chip location
	if((max_target - world.ball().position()).len() - (target - world.ball().position()).len() > Robot::MAX_RADIUS &&
		(target - world.ball().position()).len() - shortest_blocker_dist > Robot::MAX_RADIUS) {

	}
	else {
		//nowhere to chip, change chip target to open area to do chip and chase
		//target = Point(world.field().enemy_goal().x - world.field().width()/4.0, 0.0);

	}

	LOGF_INFO(u8"NET_TARGET: %1", chip_net_target);

	return target;
}

