#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/chase.h"
#include "ai/hl/stp/action/pivot.h"
#include "ai/flags.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include <algorithm>
#include <cmath>

using namespace AI::HL::STP;

namespace {
	DoubleParam alpha("Decay constant for the ball velocity", "STP/Action/shoot", 0.1, 0.0, 1.0);

	DoubleParam reduced_radius("reduced radius for calculating best shot (robot radius ratio)", "STP/Action/shoot", 0.8, 0.0, 1.0);

	DoubleParam shoot_threshold("Angle threshold (in degrees) that defines shoot accuracy, smaller is less accurate", "STP/Action/shoot", 20.0, -360.0, 360.0);
	
	DoubleParam pivot_threshold("circle radius in front of robot to start pivoting (in meters)", "STP/Action/shoot", 0.1, 0.0, 1.0);
	
	DoubleParam pass_speed("kicking speed for making a pass", "STP/Action/shoot", 5.0, 1.0, 10.0);

	// previous value of the angle returned by calc_best_shot
	double prev_best_angle = 0.0;
	Player::Ptr prev_player;
}

bool AI::HL::STP::Action::shoot(const World &world, Player::Ptr player) {
	// TODO:
	// take into account that the optimal solution may not always be the largest opening
	std::pair<Point, double> target = AI::HL::Util::calc_best_shot(world, player);

	if (target.second == 0) {
		// bad news, we are blocked
		// so try with reduced radius
		target = AI::HL::Util::calc_best_shot(world, player, reduced_radius);
	}

	Point unit_vector = Point::of_angle(player->orientation());
	Point circle_center = player->position() + Robot::MAX_RADIUS * unit_vector;

	if (!player->has_ball()) {
		double dist = (circle_center - world.ball().position()).len();
		if (dist < pivot_threshold) {
			LOG_INFO("pivoting");
			pivot(world, player, target.first);
			return false;
		}

		// ball is far away
		LOG_INFO("chase");
		if (target.second == 0) {
			// just grab the ball, don't care about orientation
			chase(world, player);
		} else {
			// grab ball and orient towards the enemy goal area
			chase(world, player, target.first);
		}
		return false;
	}

	if (target.second == 0) {
		// still blocked, just aim
		LOG_INFO("blocked, pivot");
		pivot(world, player, world.field().enemy_goal());
		return false;
	}

	double ori = (target.first - player->position()).orientation();
	double ori_diff = angle_diff(ori, player->orientation());
	double accuracy_diff = ori_diff - (target.second / 2);

#warning this is a hack that needs better fixing. check timestep
	if (prev_player != player) {
		prev_player = player;
		prev_best_angle = 0;
	}

	LOG_INFO(Glib::ustring::compose("accuracy_diff %1 shoot_thresh %2", radians2degrees(accuracy_diff), -shoot_threshold));

	if (radians2degrees(accuracy_diff) < -shoot_threshold /* && accuracy_diff < prev_best_angle */) {
		prev_best_angle = accuracy_diff;
		if (!player->chicker_ready()) {
			LOG_INFO("chicker not ready");
			return false;
		}
		LOG_INFO("kick");
		player->kick(10.0);
		return true;
	}

	LOG_INFO("aiming");
	pivot(world, player, target.first);
	prev_best_angle = accuracy_diff;
	return false;
}

bool AI::HL::STP::Action::shoot_pass(const World &world, Player::Ptr player, const Point target) {

	Point unit_vector = Point::of_angle(player->orientation());
	Point circle_center = player->position() + Robot::MAX_RADIUS * unit_vector;

	if (!player->has_ball()) {
		double dist = (circle_center - world.ball().position()).len();
		if (dist < pivot_threshold) {
			LOG_INFO("pivoting");
			pivot(world, player, target);
			return false;
		}
		
		chase(world, player, target);
		return false;
	}

	double ori = (target - player->position()).orientation();
	double ori_diff = angle_diff(ori, player->orientation());
	
#warning this is a hack that needs better fixing. check timestep
	if (prev_player != player) {
		prev_player = player;
		prev_best_angle = 0;
	}

	LOG_INFO(Glib::ustring::compose("accuracy_diff %1 shoot_thresh %2", radians2degrees(ori_diff), shoot_threshold));
	
#warning this is a hack that needs better fixing
	if (radians2degrees(ori_diff) > 2 * shoot_threshold /* && accuracy_diff < prev_best_angle */) {
		prev_best_angle = ori_diff;
		if (!player->chicker_ready()) {
			LOG_INFO("chicker not ready");
			return false;
		}
		LOG_INFO("kick");
		player->kick(pass_speed);
		return true;
	}

	LOG_INFO("aiming");
	pivot(world, player, target);
	prev_best_angle = ori_diff;
	return false;
}

#warning this is broken
bool AI::HL::STP::Action::shoot(const World &world, Player::Ptr player, const Point target, double tol, double delta) {
	player->move(target, (target - player->position()).orientation(), Point());
	player->type(AI::Flags::MoveType::CATCH);
	player->prio(AI::Flags::MovePrio::HIGH);

	Point segA = player->position();
	Point segB((world.field().total_length() + world.field().total_width()), 0);
	segB = segB.rotate(player->orientation());
	double error = lineseg_point_dist(target, segA, segB);
	if (error > tol) {
		return false;
	}
	arm(world, player, target, delta);
	return player->has_ball() && player->chicker_ready();
}

bool AI::HL::STP::Action::arm(const World &world, Player::Ptr player, const Point target, double delta) {
	double dist_max = 10.0 * (1 - std::exp(-alpha * delta)) / alpha;
	// make the robot kick as close to the target as possible
	Point robot_dir(1, 0);
	robot_dir = robot_dir.rotate(player->orientation());
	double distance = (target - world.ball().position()).dot(robot_dir);

	if (distance > dist_max) {
		player->autokick(10.0);
		return false;
	}
	double speed = alpha * distance / (1 - std::exp(-alpha * delta));
	if (speed > 10.0) {
		speed = 10.0; // can't kick faster than this
	}
	if (speed < 0) {
		speed = 0; // can't kick slower than this
	}
	player->autokick(speed);
	return true;
}

