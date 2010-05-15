#include "ai/navigator/basic_navigator.h"

#include <cmath>

basic_navigator::basic_navigator(player::ptr player, field::ptr field, ball::ptr ball, team::ptr team) : navigator(player, field, ball, team) {
	dest_initialized = false;
	outofbounds_margin = field->width() / 20.0;
	max_lookahead = 1.0;
	aggression_factor = 2;
	rotation_angle = 1.0 * PI / 180.0;
	rotation_thresh = 100 * PI / 180.0;
}

void basic_navigator::tick() {

	if(!dest_initialized) return;

	point nowdest;

	point balldirection = the_ball->position() - the_player->position();

	// if we have the ball, adjust our destination to ensure that we
	// don't take the ball out of bounds, otherwise, head to our
	// assigned destination
	if (the_player->has_ball()) {
		nowdest = clip_point(curr_dest, point(-the_field->length()/2 + outofbounds_margin, -the_field->width()/2 + outofbounds_margin),
				point(the_field->length()/2 - outofbounds_margin, the_field->width()/2 - outofbounds_margin));
	} else {
		nowdest = curr_dest;
	}

	point direction = nowdest - the_player->position();

	// at least face the ball
	if (direction.len() < 0.01) {
		if (!the_player->has_ball())
			the_player->move(the_player->position(), atan2(balldirection.y, balldirection.x));
		return;
	}

	double dirlen = direction.len();
	direction = direction / direction.len();

	point leftdirection = direction;
	point rightdirection = direction;
	
	double angle = 0.0;

	bool undiverted = true;
	bool stop = false;
	bool chooseleft;

	//it shouldn't take that many checks to get a good direction
	while (true) {
		//std::cout << "path changed" <<std::endl;

		leftdirection = direction.rotate(angle);
		rightdirection = direction.rotate(-angle);

		if (check_vector(the_player->position(), nowdest, leftdirection)) {
			chooseleft = true;
			break;
		} else if (check_vector(the_player->position(), nowdest, rightdirection)) {
			chooseleft = false;
			break;
		}

		if (angle > rotation_thresh) {
			leftdirection = rightdirection = direction;
			stop = true;
			break;
		}

		angle += rotation_angle;
	}

	undiverted = angle < 1e-5;

	// at least face the ball
	if(stop) {
		the_player->move(the_player->position(), atan2(balldirection.y, balldirection.x));
		return;
	}

	point selected_direction = (chooseleft) ? leftdirection : rightdirection;

	if (undiverted) {
		the_player->move(nowdest, atan2(balldirection.y, balldirection.x));
	} else {
		// maximum warp
		the_player->move(the_player->position() + selected_direction*std::min(dirlen,1.0), atan2(balldirection.y, balldirection.x));
	}
}

void basic_navigator::set_point(const point &destination) {
	dest_initialized = true;
	curr_dest = destination;
}

point basic_navigator::clip_point(const point& p, const point& bound1, const point& bound2) {

	double minx = std::min(bound1.x, bound2.x);
	double miny = std::min(bound1.y, bound2.y);
	double maxx = std::max(bound1.x, bound2.x);
	double maxy = std::max(bound1.y, bound2.y);

	point ret = p;

	if (p.x < minx) ret.x = minx;
	else if (p.x > maxx) ret.x = maxx;      

	if (p.y < miny) ret.y = miny;
	else if (p.y > maxy) ret.y = maxy;

	return ret;
}

bool basic_navigator::check_vector(const point& start, const point& dest, const point& direction) const {
	const point startdest = dest - start;
	const double lookahead = std::min(startdest.len(), max_lookahead);
	for (size_t i = 0; i < the_team->size() + the_team->other()->size(); i++) {
		robot::ptr rob;
		if (i >= the_team->size()) {
			rob = the_team->other()->get_robot(i - the_team->size());
		} else {
			rob = the_team->get_robot(i);
		}
		if(rob == this->the_player) continue;
		const point rp = rob->position() - start;
		const double len = rp.dot(direction);

		if (len <= 0) continue;
		const double d = sqrt(rp.dot(rp) - len*len);

		if (len < lookahead && d < 2 * aggression_factor * robot::MAX_RADIUS) {
			return false;
		}
	}

	return true;
}

