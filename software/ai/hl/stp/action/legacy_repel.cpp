#include "ai/hl/stp/action/legacy_action.h"
#include "ai/flags.h"
#include "ai/hl/stp/action/legacy_ram.h"
#include "ai/hl/stp/action/legacy_repel.h"
#include "ai/hl/stp/action/legacy_shoot.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/util.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "util/param.h"

using namespace AI::HL::STP;

namespace {
	const double FAST = 100.0;
	DoubleParam corner_repel_speed(u8"speed that repel will be kicking at in a corner", u8"AI/HL/STP/Action/repel", 3.0, 1.0, 8.0);
}

bool AI::HL::STP::Action::repel(World world, Player player) {
	bool kicked = false;

	const Field &f = world.field();
	const Point ball = world.ball().position();
	const Point diff = ball - player.position();

	// set to RAM_BALL instead of using chase
	if (!player.has_ball()) {
		Point dest = ball;
		if (dest.x < f.friendly_goal().x + Robot::MAX_RADIUS) { // avoid going inside the goal
			dest.x = f.friendly_goal().x + Robot::MAX_RADIUS;
		}
		ram(world, player, dest);
		return false;
	}

   // just shoot as long as it's not in backwards direction
   if (player.position().orientation().to_radians() < M_PI / 2 && player.position().orientation().to_radians() > -M_PI / 2) {
	   player.mp_shoot(player.position(), player.orientation(), false, BALL_MAX_SPEED);
	kicked = true;
   }
   //if we catch the ball and we're facing our own goal, we rotate to the point to the other side
   // rotating towards 0 degrees is okay, because by the time the next tick comes, this case will be false anyways.
   else
   {
	player.mp_pivot(world.ball().position(), Angle::zero());
	player.mp_dribble(player.position(), Angle::zero());
   }

   player.mp_move(world.ball().position(), diff.orientation());
   player.prio(AI::Flags::MovePrio::HIGH);

   return kicked;
}

bool AI::HL::STP::Action::corner_repel(World world, Player player) {
	const Field &f = world.field();
	const Point ball = world.ball().position();

	// if ball not in corner then just repel
	if (!Predicates::ball_in_our_corner(world) || !Predicates::ball_in_their_corner(world)) {
		return repel(world, player);
	}

	// set to RAM_BALL instead of using chase
	if (!player.has_ball()) {
		Point dest = ball;
		if (dest.x < f.friendly_goal().x + Robot::MAX_RADIUS) { // avoid going inside the goal
			dest.x = f.friendly_goal().x + Robot::MAX_RADIUS;
		}
		ram(world, player, dest);
		return false;
	}

	Evaluation::ShootData shoot_data = Evaluation::evaluate_shoot(world, player);
	if (!shoot_data.blocked) {bool goalie_repel(World world, Player player);
		return shoot_goal(world, player);
	}

	std::vector<Point> obstacles;
	EnemyTeam enemy = world.enemy_team();
	for (const Robot i : enemy) {
		obstacles.push_back(i.position());
	}

	// check circle in the middle and the centre line and find the best open spot to shoot at
	const Point p1(0.0, -f.centre_circle_radius()), p2(0.0, f.centre_circle_radius());
	std::pair<Point, Angle> centre_circle = angle_sweep_circles(player.position(), p1, p2, obstacles, Robot::MAX_RADIUS);

	const Point p3(0.0, -f.width() / 2.0), p4(0.0, f.width() / 2.0);
	std::pair<Point, Angle> centre_line = angle_sweep_circles(player.position(), p3, p4, obstacles, Robot::MAX_RADIUS);

	if (centre_circle.second > shoot_accuracy) {
		return shoot_target(world, player, centre_circle.first, corner_repel_speed);
	}
	return shoot_target(world, player, centre_line.first, corner_repel_speed);
}


bool AI::HL::STP::Action::goalie_repel(World world, Player player) {
	bool kicked = false;
	const Field &f = world.field();
	const Point ball = world.ball().position();
	const Point diff = ball - player.position();
	Point dest = ball;

	// destination used for when the ball is too far from friendly goal
	double r = world.field().defense_area_radius() + world.field().defense_area_stretch() / 2;
	// const Point destination = world.field().friendly_goal() + (ball - world.field().friendly_goal()).norm() * r;


	// set to RAM_BALL instead of using chase
	if (!player.has_ball()) {
		if (dest.x < f.friendly_goal().x + Robot::MAX_RADIUS) { // avoid going inside the goal
			dest.x = f.friendly_goal().x + Robot::MAX_RADIUS;
		}
		if ((world.ball().position() - world.field().friendly_goal()).len() > r) {
			dest = world.field().friendly_goal() + (ball - world.field().friendly_goal()).norm() * r;
		}
		goalie_ram(world, player, dest);
		return false;
	}

	//LOG_INFO("Before condition check");
	// just shoot as long as it's not in backwards direction
	if (player.orientation().angle_mod().to_radians() < M_PI / 2 &&
		player.orientation().angle_mod().to_radians() > -M_PI / 2) {
	//	LOG_INFO("should chip");
		// chip in whatever direction goalie is facing as long as its not facing back (3 was chosen arbitrarily)
		double x = world.field().friendly_goal().x + 3;
		double y = 3*tan(player.orientation().angle_mod().to_radians());
		Point shoot_pos;
		shoot_pos.x = x;
		shoot_pos.y = y;
		AI::HL::STP::Action::shoot_target(world, player, shoot_pos, 4, true);
		kicked = true;
		return true;
	}
	//if we catch the ball and we're facing our own goal, we rotate to the point to the other side
	// rotating towards 0 degrees is okay, because by the time the next tick comes, this case will be false anyways.
	else
	{	Point dest2;
		if (world.ball().position().x < f.friendly_goal().x + Robot::MAX_RADIUS) { // avoid going inside the goal
			dest2.x = f.friendly_goal().x + Robot::MAX_RADIUS;
			dest2.y = world.ball().position().y;
		} if ((world.ball().position() - world.field().friendly_goal()).len() > r) {
			dest2 = world.field().friendly_goal() + (ball - world.field().friendly_goal()).norm() * r;
		}
		if (world.ball().velocity().len() < Geom::EPS || player.has_ball())
		{
			player.mp_pivot(dest2, Angle::zero());
		}
		player.mp_move(dest2);
	}


	if (world.ball().position().x < f.friendly_goal().x + Robot::MAX_RADIUS) { // avoid going inside the goal
		Point dest3;
		dest3.x = f.friendly_goal().x + Robot::MAX_RADIUS;
		dest3.y = world.ball().position().y;
		player.mp_move(dest3);
	}
	if ((world.ball().position() - world.field().friendly_goal()).len() > r) {
		player.mp_move(world.field().friendly_goal() + (ball - world.field().friendly_goal()).norm() * r, diff.orientation());
	} else {
	player.mp_move(world.ball().position(), diff.orientation());
	}
	player.prio(AI::Flags::MovePrio::HIGH);

	return kicked;
}

