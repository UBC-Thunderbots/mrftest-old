#include "ai/hl/stp/tactic/ram.h"
#include "ai/hl/stp/action/ram.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/tactic/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::STP::Action;
using namespace AI::HL::W;

namespace {
	class Ram final : public Tactic {
		public:
			explicit Ram(World world) : Tactic(world) {
			}

		private:
			bool done() const override {
				return (player.position() - world.ball().position()).len() < AI::HL::Util::POS_CLOSE;
			}

			Player select(const std::set<Player> &players) const override {
				return select_baller(world, players, player);
			}

			void execute() override {
				// should probably use move spin action instead (or tsteal tactic in cm_ball which uses it)
				ram(world, player, world.ball().position(), Point());
			}
			Glib::ustring description() const override {
				return u8"ram";
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::ram(World world) {
	Tactic::Ptr p(new Ram(world));
	return p;
}

