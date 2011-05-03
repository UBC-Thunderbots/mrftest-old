#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/ram.h"
#include "ai/hl/stp/tactic/chase.h"
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
	 * - at least 3 players
	 * - ball under team possesion
	 * - enemy baller has clear shot to our goal!
	 *
	 * Objective:
	 * - Defend and ram the ball away
	 */
	class JustRam : public Play {
		public:
			JustRam(const AI::HL::W::World &world);

		private:
			bool invariant() const;
			bool applicable() const;
			bool done() const;
			bool fail() const;
			void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]);
			const PlayFactory &factory() const;
	};

	PlayFactoryImpl<JustRam> factory_instance("Just Ram");

	const PlayFactory &JustRam::factory() const {
		return factory_instance;
	}

	JustRam::JustRam(const World &world) : Play(world) {
	}

	bool JustRam::invariant() const {
		return Predicates::playtype(world, AI::Common::PlayType::PLAY) && Predicates::our_team_size_at_least(world, 3);
	}

	bool JustRam::applicable() const {
		return Predicates::their_ball(world) && Predicates::enemy_baller_can_shoot(world);
	}

	bool JustRam::done() const {
		return !Predicates::their_ball(world);
	}

	bool JustRam::fail() const {
		return false;
	}

	void JustRam::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]) {
		// std::Player::Ptr goalie = world.friendly_team().get(0);

		// GOALIE
		goalie_role.push_back(defend_duo_goalie(world));

		// ROLE 1
		// grab the ball if possible
		roles[0].push_back(chase(world));

		// ROLE 2
		// defend
		roles[1].push_back(defend_duo_defender(world));

		// ROLE 3 (optional)
		// ram ball
		roles[2].push_back(ram(world));

		// ROLE 4 (optional)
		// ram ball
		roles[3].push_back(ram(world));
	}
}

