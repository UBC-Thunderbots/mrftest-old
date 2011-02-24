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
		goalie_role.push_back(defend_solo_goalie(world));

		roles[0].push_back(move_wait_playtype(world, Point(-1.0, 0.0), PlayType::EXECUTE_KICKOFF_FRIENDLY));

		/*
		   roles[0].push_back();

		   roles[1].push_back();

		   roles[2].push_back();

		   roles[3].push_back();
		 */
	}
}

