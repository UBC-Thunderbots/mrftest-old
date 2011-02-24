#include "ai/hl/stp/tactic/wait_playtype.h"
#include "ai/hl/util.h"

#include <cassert>

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	class WaitPlaytype : public Tactic {
		public:
			WaitPlaytype(const World &world, const Tactic::Ptr tactic, const PlayType::PlayType playtype) : Tactic(world, true), tactic(tactic), playtype(playtype) {
				assert(tactic.is());
			}

		private:
			Tactic::Ptr tactic;
			const PlayType::PlayType playtype;
			bool done() const;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void player_changed();
			void execute();
	};

	bool WaitPlaytype::done() const {
		return world.playtype() == playtype;
	}

	void WaitPlaytype::player_changed() {
		tactic->set_player(player);
	}

	Player::Ptr WaitPlaytype::select(const std::set<Player::Ptr> &players) const {
		return tactic->select(players);
	}

	void WaitPlaytype::execute() {
		tactic->execute();
	}
}

Tactic::Ptr AI::HL::STP::Tactic::wait_playtype(const World &world, const Tactic::Ptr tactic, const PlayType::PlayType playtype) {
	const Tactic::Ptr p(new WaitPlaytype(world, tactic, playtype));
	return p;
}

