#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/action/move.h"
#include "ai/hl/util.h"
#include <algorithm>

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;
namespace Action = AI::HL::STP::Action;

namespace {
	class Move : public Tactic {
		public:
			Move(World world, const Coordinate dest) : Tactic(world), dest(dest) {
			}

		private:
			const Coordinate dest;
			Player select(const std::set<Player> &players) const;
			void execute();
			Glib::ustring description() const {
				return u8"move";
			}
	};

	Player Move::select(const std::set<Player> &players) const {
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(dest.position()));
	}

	void Move::execute() {
		Action::move(world, player, dest.position(), dest.velocity());
	}
}

Tactic::Ptr AI::HL::STP::Tactic::move(World world, const Coordinate dest) {
	Tactic::Ptr p(new Move(world, dest));
	return p;
}

