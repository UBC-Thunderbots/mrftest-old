#include "ai/hl/stp/evaluation/defense.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/tactic/penalty_goalie.h"
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
	 * - Playtype Prepare Penalty Enemy
	 *
	 * Objective:
	 * - move to Penalty positions and shoot the ball to enemy goal
	 */
	class PenaltyEnemy : public Play {
		public:
			PenaltyEnemy(const World &world);
			~PenaltyEnemy();

		private:
			bool invariant() const;
			bool applicable() const;
			bool done() const;
			bool fail() const;
			void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]);
			const PlayFactory &factory() const;
	};

	PlayFactoryImpl<PenaltyEnemy> factory_instance("Penalty Enemy");

	const PlayFactory &PenaltyEnemy::factory() const {
		return factory_instance;
	}

	PenaltyEnemy::PenaltyEnemy(const World &world) : Play(world) {
	}

	PenaltyEnemy::~PenaltyEnemy() {
	}

	bool PenaltyEnemy::invariant() const {
		return (Predicates::playtype(world, PlayType::PREPARE_PENALTY_ENEMY) || Predicates::playtype(world, PlayType::PREPARE_PENALTY_ENEMY)) && Predicates::our_team_size_at_least(world, 1);
	}

	bool PenaltyEnemy::applicable() const {
		return true;
	}

	bool PenaltyEnemy::done() const {
		return false;
	}

	bool PenaltyEnemy::fail() const {
		return false;
	}

	void PenaltyEnemy::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]) {
		// std::Player::Ptr goalie = world.Enemy_team().get(0);

		// GOALIE
		goalie_role.push_back(penalty_goalie(world));

		// ROLE 1
		// move to penalty position 1
		roles[0].push_back(move(world, Point(-0.5 * world.field().length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, 5 * Robot::MAX_RADIUS)));
		
		// ROLE 2
		// move to penalty position 2
		roles[1].push_back(move(world, Point(-0.5 * world.field().length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, 2 * Robot::MAX_RADIUS)));

		// ROLE 3
		// move to penalty position 3
		roles[2].push_back(move(world, Point(-0.5 * world.field().length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, -2 * Robot::MAX_RADIUS)));

		// ROLE 4
		// move to penalty position 4
		roles[3].push_back(move(world, Point(-0.5 * world.field().length() + RESTRICTED_ZONE_LENGTH + Robot::MAX_RADIUS, -5 * Robot::MAX_RADIUS)));
	}
}

