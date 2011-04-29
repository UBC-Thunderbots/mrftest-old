#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/util.h"
#include "util/dprint.h"
#include <glibmm.h>

using namespace AI::HL::STP::Play;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Predicates = AI::HL::STP::Predicates;

namespace {
	/**
	 * Condition:
	 * - 2 players left
	 * 
	 *
	 * Objective:
	 * - do something, shoot
	 */
	class Lone2Player : public Play {
		public:
			Lone2Player(const AI::HL::W::World &world);

		private:
			bool invariant() const;
			bool applicable() const;
			bool done() const;
			bool fail() const;
			void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]);
			const PlayFactory &factory() const;
	};

	PlayFactoryImpl<Lone2Player> factory_instance("Lone 2 Player");

	const PlayFactory &Lone2Player::factory() const {
		return factory_instance;
	}

	Lone2Player::Lone2Player(const World &world) : Play(world) {
	}

	bool Lone2Player::invariant() const {
		return Predicates::playtype(world, AI::Common::PlayType::PLAY) && Predicates::our_team_size_exactly(world, 2);
	}

	bool Lone2Player::applicable() const {
		return true;
	}

	bool Lone2Player::done() const {
		return false;
	}

	bool Lone2Player::fail() const {
		return false;
	}

	void Lone2Player::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]) {
		// std::Player::Ptr goalie = world.friendly_team().get(0);

		// GOALIE
		goalie_role.push_back(defend_solo_goalie(world));

		// ROLE 1
		// shoot
		roles[0].push_back(shoot(world));

	}
}

