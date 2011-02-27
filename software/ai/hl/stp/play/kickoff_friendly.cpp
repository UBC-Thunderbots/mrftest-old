#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/wait_playtype.h"
#include "ai/hl/stp/tactic/shoot.h"

using namespace AI::HL::STP::Play;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Predicates = AI::HL::STP::Predicates;

namespace {

	// the distance we want the players to the ball
	const double AVOIDANCE_DIST = Ball::RADIUS + Robot::MAX_RADIUS + 0.005;

	// in ball avoidance, angle between center of 2 robots, as seen from the ball
	const double AVOIDANCE_ANGLE = 2.0 * std::asin(Robot::MAX_RADIUS / AVOIDANCE_DIST);

	// distance for the offenders to be positioned away from the kicker
	const double SEPERATION_DIST = 10 * Robot::MAX_RADIUS;

	// hard coded positions for the kicker, and 2 offenders
	Point kicker_position = Point(-0.5 - Ball::RADIUS - Robot::MAX_RADIUS, 0);
	Point ready_positions[2] = { Point(-AVOIDANCE_DIST, -SEPERATION_DIST), Point(-AVOIDANCE_DIST, SEPERATION_DIST) };

	/**
	 * Condition:
	 * - It is the execute friendly kickoff play
	 *
	 * Objective:
	 * - Pass the ball to a friendly player without double touching the ball
	 */
	class KickoffFriendly : public Play {
		public:
			KickoffFriendly(const World &world);
			~KickoffFriendly();

		private:
			bool applicable() const;
			bool done() const;
			bool fail() const;
			void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]);
			const PlayFactory &factory() const;
	};

	PlayFactoryImpl<KickoffFriendly> factory_instance("Kickoff Friendly");

	const PlayFactory &KickoffFriendly::factory() const {
		return factory_instance;
	}

	KickoffFriendly::KickoffFriendly(const World &world) : Play(world) {
	}

	KickoffFriendly::~KickoffFriendly() {
	}

	bool KickoffFriendly::applicable() const {
		return Predicates::our_team_size_at_least(world, 2)
			&& (Predicates::playtype(world, PlayType::PREPARE_KICKOFF_FRIENDLY)
					|| Predicates::playtype(world, PlayType::EXECUTE_KICKOFF_FRIENDLY));
	}

	bool KickoffFriendly::done() const {
		return !Predicates::playtype(world, PlayType::EXECUTE_KICKOFF_FRIENDLY)
			&& !Predicates::playtype(world, PlayType::EXECUTE_KICKOFF_FRIENDLY);
	}

	bool KickoffFriendly::fail() const {
		return false;
	}

	void KickoffFriendly::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]) {
		goalie_role.push_back(defend_duo_goalie(world));

		roles[0].push_back(wait_playtype(world, move(world, kicker_position), PlayType::EXECUTE_KICKOFF_FRIENDLY));
		roles[0].push_back(shoot(world));

		roles[1].push_back(move(world, ready_positions[0]));

		roles[2].push_back(move(world, ready_positions[1]));
	
		roles[3].push_back(defend_duo_defender(world));
		
	}
}

