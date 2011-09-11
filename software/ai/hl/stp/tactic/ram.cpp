#include "ai/hl/stp/tactic/ram.h"
#include "ai/hl/stp/action/ram.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/tactic/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::STP::Action;
using namespace AI::HL::W;

namespace {
	class Ram : public Tactic {
		public:
			Ram(const World &world) : Tactic(world) {
			}

		private:
			bool done() const {
				return (player->position() - world.ball().position()).len() < AI::HL::Util::POS_CLOSE;
			}

			Player::Ptr select(const std::set<Player::Ptr> &players) const {
				return select_baller(world, players, player);
			}

			void execute() {
				// should probably use move spin action instead (or tsteal tactic in cm_ball which uses it)
				ram(world, player, world.ball().position(), Point());
			}
			Glib::ustring description() const {
				return "ram";
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::ram(const World &world) {
	const Tactic::Ptr p(new Ram(world));
	return p;
}

