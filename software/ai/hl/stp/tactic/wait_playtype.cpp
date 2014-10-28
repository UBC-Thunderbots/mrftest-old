#include "ai/hl/stp/tactic/wait_playtype.h"
#include "ai/hl/util.h"
#include <cassert>
#include <utility>

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	class WaitPlaytype final : public Tactic {
		public:
			explicit WaitPlaytype(World world, Tactic::Ptr &&tactic, const AI::Common::PlayType playtype) : Tactic(world, true), tactic(std::move(tactic)), playtype(playtype) {
				assert(this->tactic);
			}

		private:
			Tactic::Ptr tactic;
			const AI::Common::PlayType playtype;
			bool done() const override;
			Player select(const std::set<Player> &players) const override;
			void player_changed() override;
			void execute() override;
			Glib::ustring description() const override {
				return Glib::ustring::compose(u8"wait-playtype, %1", tactic->description());
			}
	};

	bool WaitPlaytype::done() const {
		return world.playtype() == playtype;
	}

	void WaitPlaytype::player_changed() {
		tactic->set_player(player);
	}

	Player WaitPlaytype::select(const std::set<Player> &players) const {
		return tactic->select(players);
	}

	void WaitPlaytype::execute() {
		tactic->execute();
	}
}

Tactic::Ptr AI::HL::STP::Tactic::wait_playtype(World world, Tactic::Ptr &&tactic, const AI::Common::PlayType playtype) {
	Tactic::Ptr p(new WaitPlaytype(world, std::move(tactic), playtype));
	return p;
}

