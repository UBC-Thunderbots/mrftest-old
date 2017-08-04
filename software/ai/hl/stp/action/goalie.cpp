#include "goalie.h"

#include "ai/hl/stp/action/action.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/flags.h"
#include "ai/hl/stp/action/ram.h"
#include "ai/hl/stp/evaluation/ball_threat.h"
#include "ai/hl/util.h"
#include "chip.h"
#include "geom/util.h"
#include "repel.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace AI::HL::STP;
using namespace Geom;
using AI::HL::STP::Evaluation::BallThreat;

namespace {
	const double FAST = 100.0;
	DoubleParam lone_goalie_dist(u8"Lone Goalie: distance to goal post (m)", u8"AI/HL/STP/Action/Goalie", 0.5, 0.05, 1.0);
	DoubleParam goalie_repel_dist(u8"Distance the goalie should repel the ball in robot radius", u8"AI/HL/STP/Action/Goalie", 4.0, 1.0, 6.0);
	DoubleParam goalieChipPower(u8"How far the goalie chips when it clears the ball", u8"AI/HL/STP/Action/Goalie", 1.5, 0.0, 6.0);
	DoubleParam ballVelocityThreshold(u8"How slow the ball must be to kick it away", u8"AI/HL/STP/Action/Goalie", 0.15, 0.0, 8.0);
}

//goalie moves in the direction towards the ball within the lone_goalie_dist from the goal post
void AI::HL::STP::Action::lone_goalie(caller_t& ca, World world, Player player) {
//  OLD VERSION
//	Circle circle = Circle(Point(world.field().friendly_goal()), lone_goalie_dist);
//	Point centerOfGoal = world.field().friendly_goal();
//	Point targetDir = world.ball().position() - centerOfGoal;
//	std::vector<Point> targets = line_circle_intersect(circle.origin, circle.radius, centerOfGoal, world.ball().position());
//
//	Point target;
//	if(targets.empty() || targets.size() >= 2) {
//		LOG_INFO("EMPTY VECTOR");
//		target = centerOfGoal + targetDir.norm(lone_goalie_dist);
//	}else {
//		target = targets[0];
//	}
//
//	if (target.x < world.field().friendly_goal().x + Robot::MAX_RADIUS + 0.05) {
//		// avoid going inside the goal
//		target.x = world.field().friendly_goal().x + Robot::MAX_RADIUS + 0.05;
//	}
//
//	player.display_path(std::vector<Point> {target});
//	player.move_move(target, targetDir.orientation(), 0);

	bool ballInFrontOfRobots = world.ball().position().x > player.position().x + 0.03;

	LOGF_INFO(u8"ball vel: %1", world.ball().velocity().len());
	if(dist(world.ball().position(), Seg(world.field().friendly_goal_boundary().first, world.field().friendly_goal_boundary().second)) < world.field().defense_area_radius() &&
			world.ball().velocity().len() < ballVelocityThreshold && ballInFrontOfRobots) {
		Action::shoot_target(ca, world, player, world.field().enemy_goal(), goalieChipPower, true);
	}else {
		Seg goalLine = Seg(world.field().friendly_goal_boundary().first, world.field().friendly_goal_boundary().second);
		Seg shotLine = Seg(world.ball().position(), world.ball().position() + world.ball().velocity().norm(100));
		std::vector<Point> intersect = line_intersect(goalLine, shotLine);
		Point base = world.field().friendly_goal();

		if(!intersect.empty() && world.ball().velocity().len() > ballVelocityThreshold) {
//			LOGF_INFO(u8"Test intersect: %1", intersect[0]);
			// add 0.05 so even if the we think the ball is a bit off the net treat is as being on net just in case
			if(std::fabs(intersect[0].y) <= std::fabs(world.field().friendly_goal_boundary().first.y) + 0.05) {
				// ball is shot at net
				base = intersect[0];
			}else {
				// leave base at center of nets
			}
		}

		// clamp the y value so the robot stays within the goalposts
		double y = base.y;
		if(y >= 0) {
			y = std::min(y, std::fabs(world.field().friendly_goal_boundary().first.y));
		}else {
			y = std::max(y, -std::fabs(world.field().friendly_goal_boundary().first.y));
		}
		base.y = y;

		Point target = base + (world.ball().position() - base).norm(lone_goalie_dist);

		if (target.x < world.field().friendly_goal().x + Robot::MAX_RADIUS + 0.01) {
			// avoid going inside the goal
			target.x = world.field().friendly_goal().x + Robot::MAX_RADIUS + 0.01;
		}

		player.display_path(std::vector<Point> {target});
		player.move_move(target, (world.ball().position() - target).orientation(), 0);
	}
}

void AI::HL::STP::Action::goalie_move(caller_t& ca, World world, Player player, Point dest) {

	for (auto i : world.enemy_team()) {
	// If enemy is in our defense area, go touch them so we get penalty kick
		if(AI::HL::Util::point_in_friendly_defense(world.field() , i.position())) {
			player.avoid_distance(AI::Flags::AvoidDistance::SHORT);
			player.move_move(i.position());
			return;
		}
	}
	player.move_move(dest, (world.ball().position() - player.position()).orientation());
	return;

	// if ball is inside the defense area or just too close, repel!!!!
	if ((AI::HL::Util::point_in_friendly_defense(world.field(), world.ball().position()) ||
		(world.ball().position() - player.position()).len() < goalie_repel_dist * Robot::MAX_RADIUS) &&
		world.playtype() != AI::Common::PlayType::STOP)
	{
		goalie_repel(ca, world, player);
		return;
	}
	// check if ball is heading towards our goal
	if (Evaluation::ball_on_net(world)) {
		// goalie block position
		Point goal_pos = Evaluation::goalie_shot_block(world, player);
		// avoid going inside of goal
		if (goal_pos.x < world.field().friendly_goal().x + Robot::MAX_RADIUS) {
					// avoid going outside the goal??
			goal_pos.x = world.field().friendly_goal().x + Robot::MAX_RADIUS;
		}

		goalie_ram(ca, world, player, goal_pos);

	} else {
//		if(pow( pow(dest.x, 2) + pow(dest.y , 2) , 0.5 ) > 0.5 - Robot::MAX_RADIUS) {
//			double angle = atan(dest.y/dest.x);
//			dest.x = 0.3*cos(angle) + world.fi;
//			dest.y = 0.3*sin(angle);
//		}
//		if ((goal_center - dest).len() > 0.5) {
//			dest = goal_center + (dest - goal_center).norm() * 0.5;
//		}
		goalie_ram(ca, world, player, dest);
	}
}

void AI::HL::STP::Action::goalie_move_direct(caller_t& ca, World world, Player player, const Point dest) {
	Action::move(ca, world, player, dest, (world.ball().position() - player.position()).orientation());
}
