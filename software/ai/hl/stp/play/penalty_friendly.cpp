#include "ai/hl/stp/evaluation/defense.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/tactic/penalty_shoot.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/wait_playtype.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/util.h"
#include "util/dprint.h"
#include <glibmm.h>

using namespace AI::HL::STP::Play;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Predicates = AI::HL::STP::Predicates;

namespace {
	const double PENALTY_MARK_LENGTH = 0.45;
	const double RESTRICTED_ZONE_LENGTH = 0.85;
	/**
	 * Condition:
	 * - Playtype Prepare Penalty Friendly
	 *
	 * Objective:
	 * - move to Penalty positions and shoot the ball to enemy goal
	 */
	class PenaltyFriendly : public Play {
		public:
			PenaltyFriendly(const World &world);
			~PenaltyFriendly();

		private:
			bool applicable() const;
			bool done() const;
			bool fail() const;
			void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]);
			const PlayFactory &factory() const;
	};

	PlayFactoryImpl<PenaltyFriendly> factory_instance("Penalty Friendly");

	const PlayFactory &PenaltyFriendly::factory() const {
		return factory_instance;
	}

	PenaltyFriendly::PenaltyFriendly(const World &world) : Play(world) {
	}

	PenaltyFriendly::~PenaltyFriendly() {
	}

	bool PenaltyFriendly::applicable() const {
		return Predicates::playtype(world, PlayType::PREPARE_PENALTY_FRIENDLY) && Predicates::our_team_size_at_least(world, 2);
	}

	bool PenaltyFriendly::done() const {
		return !(Predicates::playtype(world, PlayType::EXECUTE_PENALTY_FRIENDLY) || Predicates::playtype(world, PlayType::PREPARE_PENALTY_FRIENDLY)) ;
	}

	bool PenaltyFriendly::fail() const {
		return false;
	}

	void PenaltyFriendly::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]) {
		// std::Player::Ptr goalie = world.friendly_team().get(0);

		// GOALIE
		goalie_role.push_back(defend_solo_goalie(world));

		// ROLE 1
		// move to shooting position and shoot
		roles[0].push_back(wait_playtype(world, move(world, Point(0.5 * world.field().length() - PENALTY_MARK_LENGTH - Robot::MAX_RADIUS, 0)), PlayType::EXECUTE_PENALTY_FRIENDLY));
		roles[0].push_back(penalty_shoot(world));
		
		// ROLE 2
		// move to penalty position 1
		roles[1].push_back(move(world, Point(0.5 * world.field().length() - RESTRICTED_ZONE_LENGTH - Robot::MAX_RADIUS, -5 * Robot::MAX_RADIUS)));

		// ROLE 3
		// move to penalty position 2
		roles[2].push_back(move(world, Point(0.5 * world.field().length() - RESTRICTED_ZONE_LENGTH - Robot::MAX_RADIUS, 5 * Robot::MAX_RADIUS)));

		// ROLE 4
		// move to penalty position 3
		roles[3].push_back(move(world, Point(0.5 * world.field().length() - RESTRICTED_ZONE_LENGTH - 5 * Robot::MAX_RADIUS, 0)));
	}
}

