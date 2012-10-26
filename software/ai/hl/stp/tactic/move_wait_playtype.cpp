#include "ai/hl/stp/tactic/move_wait_playtype.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;

namespace Action = AI::HL::STP::Action;

namespace {
	class MoveWaitPlaytype : public Tactic {
		public:
			MoveWaitPlaytype(World world, Coordinate dest, AI::Common::PlayType playtype) : Tactic(world, true), dest(dest), playtype(playtype) {
			}

		private:
			const Coordinate dest;
			const AI::Common::PlayType playtype;
			bool done() const;
			Player select(const std::set<Player> &players) const;
			void execute();
			Glib::ustring description() const {
				return "move-wait-playtype";
			}
	};

	bool MoveWaitPlaytype::done() const {
		return world.playtype() == playtype && (player->position() - dest.position()).len() < AI::HL::Util::POS_CLOSE;
	}

	Player MoveWaitPlaytype::select(const std::set<Player> &players) const {
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(dest.position()));
	}

	void MoveWaitPlaytype::execute() {
		Action::move(world, player, dest.position());
	}
}

Tactic::Ptr AI::HL::STP::Tactic::move_wait_playtype(World world, Coordinate dest, AI::Common::PlayType playtype) {
	Tactic::Ptr p(new MoveWaitPlaytype(world, dest, playtype));
	return p;
}

