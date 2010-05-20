#include "ai/navigator/robot_navigator.h"
#include "geom/angle.h"
#include "ai/util.h"

#include <iostream>
#include <cstdlib>

namespace {

#warning magic constants
	const double AVOID_MULT = 1.0;
	const double AVOID_CONST = 1.0;
	const double ROTATION_THRESH = 100.0 * PI / 180.0;
	const double ROTATION_STEP = 1.0 * PI / 180.0;
}

robot_navigator::robot_navigator(player::ptr player, world::ptr world) : the_player(player), the_world(world), dest_initialized(false), ori_initialized(false), outofbounds_margin(the_world->field().width() / 20.0) {
	lookahead_max = robot::MAX_RADIUS * 10;
}

double robot_navigator::get_avoidance_factor() const {
	return AVOID_CONST + AVOID_MULT * the_player->est_velocity().len();
}

void robot_navigator::tick() {
	const ball::ptr the_ball(the_world->ball());
	const field &the_field(the_world->field());

	// TODO: face towards the ball and stay in same place
	if (!dest_initialized) return;

#warning TODO bound the ball if set by flag
	// BY DEFAULT, NAVIGATOR ALLOWS ROBOT ROAM FREE
	// if we have the ball, adjust our destination to ensure that we
	// don't take the ball out of bounds, otherwise, head to our
	// assigned destination
	// point wantdest;
	// if (the_player->has_ball()) {
	// wantdest = ai_util::clip_point(curr_dest, point(-the_field.length()/2 + outofbounds_margin, -the_field.width()/2 + outofbounds_margin),
	//point(the_field.length()/2 - outofbounds_margin, the_field.width()/2 - outofbounds_margin));
	//} else {
	//wantdest = curr_dest;
	//}

	const point balldist = the_ball->position() - the_player->position();
	const point wantdest = curr_dest;
	const double wantori = (ori_initialized) ? curr_ori : atan2(balldist.y, balldist.x);
	const double distance = (wantdest - the_player->position()).len();

	// reset orientation settings.
	ori_initialized = false;

	// at least face the ball
	if (distance < ai_util::POS_CLOSE) {
		if (balldist.len() > ai_util::POS_CLOSE) the_player->move(the_player->position(), wantori);
		return;
	}

	const point direction = (wantdest - the_player->position()).norm();

	point leftdirection = direction;
	point rightdirection = direction;

	double angle = 0.0;

	bool stop = false;
	bool chooseleft;

	//it shouldn't take that many checks to get a good direction
	while (true) {

		leftdirection = direction.rotate(angle);
		rightdirection = direction.rotate(-angle);

		if (check_vector(the_player->position(), wantdest, leftdirection)) {
			chooseleft = true;
			break;
		} else if (check_vector(the_player->position(), wantdest, rightdirection)) {
			chooseleft = false;
			break;
		}

		if (angle > ROTATION_THRESH) {
			leftdirection = rightdirection = direction;
			stop = true;
			break;
		}
		angle += ROTATION_STEP;
	}

	if(stop) {
		the_player->move(the_player->position(), wantori);
		return;
	}

	const point selected_direction = (chooseleft) ? leftdirection : rightdirection;

	if (angle < ai_util::ORI_CLOSE) {
		the_player->move(wantdest, wantori);
	} else {
		// maximum warp
		the_player->move(the_player->position() + selected_direction * std::min(distance, 1.0), wantori);
	}
}

void robot_navigator::set_point(const point &destination) {
	dest_initialized = true;
	curr_dest = destination;
}

void robot_navigator::set_orientation(const double &orientation) {
	ori_initialized = true;
	curr_ori = orientation;
}

#warning Please do something about these functions
#warning The flags should be refactored somehow

/**
  get whether the robot avoid it's own goal
 */
bool robot_navigator::robot_avoids_goal() {
	return false;
}

/**
  make the robot avoid it's own goal
  \param avoid whether to avoid it's own goal
 */
void robot_navigator::set_robot_avoids_goal(bool) {
}

/**
  get whether the robot is set to stay on it's own half
 */
bool robot_navigator::robot_stays_on_own_half() {
	return false;
}

/**
  make the robot stay on it's own half
  \param avoid whether to make robot stay on it's own half
 */
void robot_navigator::set_robot_stays_on_own_half(bool) {
}

/**
  get whether the robot avoid it's opponents goal
 */
bool robot_navigator::robot_stays_away_from_opponent_goal() {
	return false;
}

/**
  make the robot avoid it's opponents goal
  \param avoid whether to avoid it's opponents goal
 */
void robot_navigator::set_robot_stays_away_from_opponent_goal(bool) {
}

// TODO: use the util functions
bool robot_navigator::check_vector(const point& start, const point& dest, const point& direction) const {
	const ball::ptr the_ball(the_world->ball());
	const point startdest = dest - start;
	const double lookahead = std::min(startdest.len(), lookahead_max);

	if(abs(direction.len() - 1.0) > ai_util::POS_CLOSE) {
		std::cerr << " Direction not normalized! " << direction.len() << std::endl;
	}

	const team * const teams[2] = { &the_world->friendly, &the_world->enemy };
	for (unsigned int i = 0; i < 2; ++i) {
		for (unsigned int j = 0; j < teams[i]->size(); ++j) {
			const robot::ptr rob(teams[i]->get_robot(j));
			if(rob == this->the_player) continue;
			const point rp = rob->position() - start;
			const double proj = rp.dot(direction);
			const double perp = sqrt(rp.dot(rp) - proj * proj);

			if (proj <= 0) continue;

			if (proj < lookahead && perp < get_avoidance_factor() * (robot::MAX_RADIUS * 2)) {
				return false;
			}
		}
	}

	if(avoid_ball) {
		const point ballvec = the_ball->position() - start;
		double proj = ballvec.dot(direction);
		if (proj > 0) {
			double perp = sqrt(ballvec.dot(ballvec) - proj * proj);
			if (proj < lookahead && perp < get_avoidance_factor() * (robot::MAX_RADIUS + ball::RADIUS)) {
				//std::cout << "Checked FALSE" << std::endl;
				return false;
			}
		}
	}

	//std::cout << "Checked TRUE" << std::endl;
	return true;
}

