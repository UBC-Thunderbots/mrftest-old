#include "ai/hl/stp/param.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/util.h"
#include "ai/util.h"
#include "ai/hl/stp/world.h"
#include <algorithm>
#include "ai/hl/stp/tactic/move_stop.h"
#include "ai/hl/stp/action/stop.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Action = AI::HL::STP::Action;


namespace {
	StopLocations stop_locations;
	const double AVOIDANCE_DIST = AI::Util::BALL_STOP_DIST + Robot::MAX_RADIUS + Ball::RADIUS + 0.05;

	// in ball avoidance, angle between center of 2 robots, as seen from the ball
	const Angle AVOIDANCE_ANGLE = 2.0 * Angle::of_radians(std::asin(Robot::MAX_RADIUS / AVOIDANCE_DIST));
	
	const unsigned int NUM_PLAYERS = AI::HL::STP::TEAM_MAX_SIZE -1;

	class MoveStop final : public Tactic {
		
		public:
			explicit MoveStop(World world, std::size_t player_index) : Tactic(world), player_index(player_index) { }

		private:
			Player select(const std::set<Player> &players) const override;

			void execute(caller_t& caller) override;

			std::size_t player_index;

			Glib::ustring description() const override {
				return u8"move_stop";
			}
	};

	Player MoveStop::select(const std::set<Player> &players) const {
		std::vector<Point> positions = stop_locations(world);
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(positions[player_index]));
	}

	void MoveStop::execute(caller_t& caller) {
		player().flags(player().flags() | AI::Flags::MoveFlags::AVOID_BALL_MEDIUM);
		std::vector<Point> positions = stop_locations(world);
		AI::HL::STP::Action::move(caller, world, player(), positions[player_index]);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::move_stop(World world, std::size_t player_index) {
	Tactic::Ptr p(new MoveStop(world, player_index));
	return p;
}

std::vector<Point> StopLocations::compute(World world) {
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
		def1.y += Robot::MAX_RADIUS * 1.5;
		Point def2 = def;
		def2.y -= Robot::MAX_RADIUS * 1.5;
		
		// ball in our corner!
		if (world.ball().position().x <= -world.field().length() / 4 && std::fabs(world.ball().position().y) > world.field().goal_width() / 2 + world.field().defense_area_radius()) {
			if (world.ball().position().y < 0) {
				def1.y -= Robot::MAX_RADIUS * 3;
				def1.x += Robot::MAX_RADIUS * 3;
			} else {
				def2.y += Robot::MAX_RADIUS * 3;
				def2.x += Robot::MAX_RADIUS * 3;
			}
		}

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
