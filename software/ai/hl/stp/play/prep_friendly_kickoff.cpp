#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/tactic/defend.h"

using namespace AI::HL::STP::Play;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Predicates = AI::HL::STP::Predicates;

namespace {
	/**
	 * Condition:
	 * - It is the prep friendly kickoff play
	 *
	 * Objective:
	 * - Move into formation for the kickoff
	 */
	class PrepFriendlyKickoff : public Play {
		public:
			PrepFriendlyKickoff(World &world);
			~PrepFriendlyKickoff();

		private:
			bool applicable() const;
			bool done() const;
			bool fail() const;
			void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]);
			const PlayFactory &factory() const;
	};

	PlayFactoryImpl<PrepFriendlyKickoff> factory_instance("Prep Friendly Kickoff");

	const PlayFactory &PrepFriendlyKickoff::factory() const {
		return factory_instance;
	}

	PrepFriendlyKickoff::PrepFriendlyKickoff(World &world) : Play(world) {
	}

	PrepFriendlyKickoff::~PrepFriendlyKickoff() {
	}

	bool PrepFriendlyKickoff::applicable() const {
		return Predicates::playtype(world, PlayType::PREPARE_KICKOFF_FRIENDLY);
	}

	bool PrepFriendlyKickoff::done() const {
		return !Predicates::playtype(world, PlayType::PREPARE_KICKOFF_FRIENDLY);
	}

	bool PrepFriendlyKickoff::fail() const {
		return false;
	}

	void PrepFriendlyKickoff::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]) {
		goalie_role.push_back(defend_solo_goalie(world));

		/*
		   roles[0].push_back();

		   roles[1].push_back();

		   roles[2].push_back();

		   roles[3].push_back();
		 */
	}
}

