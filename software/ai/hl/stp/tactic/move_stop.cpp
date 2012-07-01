#include "ai/hl/stp/param.h"
#include "ai/hl/stp/tactic/move_stop.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/util.h"
#include "ai/util.h"
#include "ai/hl/stp/world.h"
#include <algorithm>

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;
namespace Action = AI::HL::STP::Action;

StopLocations AI::HL::STP::Tactic::stop_locations;

namespace {
	// The closest distance players allowed to the ball
	// DO NOT make this EXACT, instead, add a little tolerance!
	const double AVOIDANCE_DIST = AI::Util::BALL_STOP_DIST + Robot::MAX_RADIUS + Ball::RADIUS + 0.005;

	// in ball avoidance, angle between center of 2 robots, as seen from the ball
	const Angle AVOIDANCE_ANGLE = 2.0 * Angle::of_radians(std::asin(Robot::MAX_RADIUS / AVOIDANCE_DIST));
	
	const unsigned int NUM_PLAYERS = AI::HL::STP::TEAM_MAX_SIZE -1;

	class MoveStop : public Tactic {
		public:
			MoveStop(const World &world, std::size_t playerIndex) : Tactic(world), player_index(playerIndex) {
			}

		private:
			const std::size_t player_index;
			Player::Ptr selected_player;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			Glib::ustring description() const {
				return "move-stop";
			}
	};

	Player::Ptr MoveStop::select(const std::set<Player::Ptr> &players) const {
		std::vector<Point> positions = stop_locations(world);

		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(positions[player_index]));
	}

	void MoveStop::execute() {
		std::vector<Point> positions = stop_locations(world);

		Action::move(world, player, positions[player_index]);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::move_stop(const World &world, std::size_t player_index) {
	Tactic::Ptr p(new MoveStop(world, player_index));
	return p;
}

std::vector<Point> StopLocations::compute(const World &world) {
	// draw a circle of radius 50cm from the ball
	Point ball_pos = world.ball().position();
	Point goal_pos = world.field().friendly_goal();
	Point start;

	std::vector<Point> positions;

	bool in_defense = AI::HL::Util::point_in_friendly_defense(world.field(), ball_pos);

	Point def = AI::HL::Util::crop_point_to_field(world.field(), (ball_pos + goal_pos) * 0.5);

	if (in_defense) {
		// if the goal is inside the circle,
		// then shoot the ray from the enemy goal

		start = Point(ball_pos.x + AVOIDANCE_DIST, ball_pos.y);
	} else {
		// otherwise
		// draw a ray from the center of friendly goal to the ball,
		// and the intersection shall be the start point.

		Point ray = (goal_pos - ball_pos).norm();
		start = ball_pos + ray * AVOIDANCE_DIST;

		Point def1 = def;
		def1.y += Robot::MAX_RADIUS * 1.25;
		Point def2 = def;
		def2.y -= Robot::MAX_RADIUS * 1.25;
		
		positions.push_back(def1);
		positions.push_back(def2);
	}

	// calculate angle between robots
	const Angle delta_angle = AVOIDANCE_ANGLE + separation_angle;

	const Point shoot = (start - ball_pos);

	// the parity determines left or right
	// we only want one of angle = 0, so start at w = 1
	int w = 1;
	for (std::size_t i = 0; i < NUM_PLAYERS; ++i) {
		Angle angle = delta_angle * (w / 2) * ((w % 2) ? 1 : -1);
		Point p = ball_pos + shoot.rotate(angle);
		++w;

		positions.push_back(AI::HL::Util::crop_point_to_field(world.field(), p));
	}

	return positions;
}

