#include "ai/hl/stp/evaluation/defense.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/move_wait_playtype.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/util.h"
#include "util/dprint.h"
#include <glibmm.h>

using namespace AI::HL::STP::Play;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Predicates = AI::HL::STP::Predicates;

namespace {

	// the distance we want the players to the ball
	const double AVOIDANCE_DIST = 0.50 + Ball::RADIUS + Robot::MAX_RADIUS + 0.005;

	// in ball avoidance, angle between center of 2 robots, as seen from the ball
	const double AVOIDANCE_ANGLE = 2.0 * asin(Robot::MAX_RADIUS / AVOIDANCE_DIST);

	DoubleParam separation_angle("kickoff: angle to separate players (degrees)", 40, 0, 80);
	
	/**
	 * Condition:
	 * - Playtype Free Kick Enemy
	 *
	 * Objective:
	 * - Handle Enemy Free Kick
	 */
	class FreeKickEnemy : public Play {
		public:
			FreeKickEnemy(const World &world);
			~FreeKickEnemy();

		private:
			bool applicable() const;
			bool done() const;
			bool fail() const;
			void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]);
			const PlayFactory &factory() const;
	};

	PlayFactoryImpl<FreeKickEnemy> factory_instance("Free Kick Enemy");

	const PlayFactory &FreeKickEnemy::factory() const {
		return factory_instance;
	}

	FreeKickEnemy::FreeKickEnemy(const World &world) : Play(world) {
	}

	FreeKickEnemy::~FreeKickEnemy() {
	}

	bool FreeKickEnemy::applicable() const {
		return (Predicates::playtype(world, PlayType::PREPARE_KICKOFF_ENEMY) || Predicates::playtype(world, PlayType::EXECUTE_KICKOFF_ENEMY)) && Predicates::our_team_size_at_least(world, 1);
	}

	bool FreeKickEnemy::done() const {
		return !(Predicates::playtype(world, PlayType::PREPARE_KICKOFF_ENEMY) || Predicates::playtype(world, PlayType::EXECUTE_KICKOFF_ENEMY));
	}

	bool FreeKickEnemy::fail() const {
		return false;
	}

	void FreeKickEnemy::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]) {
		// std::Player::Ptr goalie = world.Enemy_team().get(0);
		// GOALIE
		goalie_role.push_back(defend_duo_goalie(world));

		// ROLE 1
		// defend
		roles[0].push_back(defend_duo_defender(world));
		
		// draw a circle of radius 50cm from the ball
		Point ball_pos = world.ball().position();

		// calculate angle between robots
		const double delta_angle = AVOIDANCE_ANGLE + separation_angle * M_PI / 180.0;

		// a ray that shoots from the center to friendly goal.
		const Point shoot = Point(-1, 0) * AVOIDANCE_DIST;

		// do matching
		std::vector<Point> positions;

		switch (world.friendly_team().size()-2) {
			case 3:
				positions.push_back(shoot);

			case 2:
				positions.push_back(shoot.rotate(delta_angle));

			case 1:
				positions.push_back(shoot.rotate(-delta_angle));

			default:
				break;
		}
				
		// ROLE 2
		// move to offender position 1
		roles[1].push_back(move(world, positions[0]));
		
		// ROLE 3
		// move to offender position 2
		roles[2].push_back(move(world, positions[1]));
		
		// ROLE 4
		// move to offender position 3
		roles[3].push_back(move(world, positions[2]));
	}
}

