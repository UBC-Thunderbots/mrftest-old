#include "ai/hl/stp/play/play.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/move_stop.h"
#include "ai/hl/stp/tactic/wait_playtype.h"

using namespace AI::HL::STP::Play;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
namespace Predicates = AI::HL::STP::Predicates;

namespace {

	/**
	 * Condition:
	 * - It is the stop play
	 *
	 * Objective:
	 * - Handle the stop play
	 */
	class Stop : public Play {
		public:
			Stop(const World &world);
			~Stop();

		private:
			bool invariant() const;
			bool applicable() const;
			bool done() const;
			bool fail() const;
			void assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]);
			const PlayFactory &factory() const;
	};

	PlayFactoryImpl<Stop> factory_instance("Stop");

	const PlayFactory &Stop::factory() const {
		return factory_instance;
	}

	Stop::Stop(const World &world) : Play(world) {
	}

	Stop::~Stop() {
	}

	bool Stop::invariant() const {
		return Predicates::playtype(world, AI::Common::PlayType::STOP);
	}

	bool Stop::applicable() const {
		return true;
	}

	bool Stop::done() const {
		return false;
	}

	bool Stop::fail() const {
		return false;
	}

	void Stop::assign(std::vector<Tactic::Ptr> &goalie_role, std::vector<Tactic::Ptr>(&roles)[4]) {
		goalie_role.push_back(defend_duo_goalie(world));

		// doesn't matter what the playtype we are waiting for is here, we just need an active tactic
		roles[0].push_back(wait_playtype(world, move_stop(world, 0), AI::Common::PlayType::PLAY));

		roles[1].push_back(move_stop(world, 1));

		roles[2].push_back(move_stop(world, 2));

		roles[3].push_back(move_stop(world, 3));
	}
}

