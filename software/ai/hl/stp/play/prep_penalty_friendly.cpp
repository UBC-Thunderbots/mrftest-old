#include "ai/hl/stp/evaluation/defense.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/lone_goalie.h"
#include "ai/hl/util.h"
#include "util/dprint.h"
#include <glibmm.h>

using namespace AI::HL::STP::Play;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::STP::Predicates;
using namespace AI::HL::W;
using AI::HL::STP::Enemy;
using AI::HL::STP::Evaluation::ConeDefense;

namespace {

	const double PENALTY_MARK_LENGTH = 0.45;
	const double RESTRICTED_ZONE_LENGTH = 0.85;
	/**
	 * Condition:
	 * - Playtype Prepare Penalty Friendly
	 *
	 * Objective:
	 * - shoot the ball to enemy goal
	 */
	class PrepPenaltyFriendly : public Play {
		public:
			PrepPenaltyFriendly(AI::HL::W::World &world);
			~PrepPenaltyFriendly();

		private:
			ConeDefense cone_defense;
			bool applicable() const;
			bool done() const;
			bool fail() const;
			void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr> (&roles)[4]);
			const PlayFactory& factory() const;
	};

	PlayFactoryImpl<PrepPenaltyFriendly> factory_instance("Prepare Penalty Friendly");

	const PlayFactory& PrepPenaltyFriendly::factory() const {
		return factory_instance;
	}

	PrepPenaltyFriendly::PrepPenaltyFriendly(World &world) : Play(world), cone_defense(*this, world) {
	}

	PrepPenaltyFriendly::~PrepPenaltyFriendly() {
	}

	bool PrepPenaltyFriendly::applicable() const {
		return playtype(PlayType::PREPARE_PENALTY_FRIENDLY)->evaluate(world)
			&& our_team_size_at_least(1)->evaluate(world);
	}

	bool PrepPenaltyFriendly::done() const {
		return !playtype(PlayType::PREPARE_PENALTY_FRIENDLY)->evaluate(world);
	}

	bool PrepPenaltyFriendly::fail() const {
		return false;
	}

	void PrepPenaltyFriendly::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr> (&roles)[4]) {
		// std::Player::Ptr goalie = world.friendly_team().get(0);

		// GOALIE
		goalie_role.push_back(lone_goalie(world));

		// ROLE 1
		// move to shooting position
		roles[0].push_back(move(world, Point(0.5 * world.field().length() - PENALTY_MARK_LENGTH - Robot::MAX_RADIUS, 0)));

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

