#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/move_wait_playtype.h"

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
			PrepFriendlyKickoff(const World &world);
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

	PrepFriendlyKickoff::PrepFriendlyKickoff(const World &world) : Play(world) {
	}

	PrepFriendlyKickoff::~PrepFriendlyKickoff() {
	}

	bool PrepFriendlyKickoff::applicable() const {
		return Predicates::our_team_size_at_least(world, 2) && Predicates::playtype(world, PlayType::PREPARE_KICKOFF_FRIENDLY);
	}

	bool PrepFriendlyKickoff::done() const {
		// TODO: write something here!
		return false;
	}

	bool PrepFriendlyKickoff::fail() const {
		return !(Predicates::playtype(world, PlayType::PREPARE_KICKOFF_FRIENDLY)
				|| Predicates::playtype(world, PlayType::EXECUTE_KICKOFF_FRIENDLY)
				|| Predicates::playtype(world, PlayType::PLAY));
	}

	void PrepFriendlyKickoff::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]) {

		// GOALIE
		goalie_role.push_back(defend_solo_goalie(world));

		// ROLE 1
		// get ready for kickoff
		roles[0].push_back(move_wait_playtype(world, Point(-1.0, 0.0), PlayType::EXECUTE_KICKOFF_FRIENDLY));

		/*
		   roles[1].push_back();

		   roles[2].push_back();

		   roles[3].push_back();
		 */
	}
}

