#include "ai/hl/stp/tactic/ram.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	class Ram : public Tactic {
		public:
			Ram(World &world) : Tactic(world) {
			}

		private:
			
			bool done() const {
				return (player->position() - world.ball().position()).len() < AI::HL::Util::POS_CLOSE;
			}

			double score(Player::Ptr player) const {
				return 1.0 / (1.0 + (player->position() - world.ball().position()).len());
			}

			void execute() {
				// TODO: flags
				player->move(world.ball().position(), (world.ball().position() - player->position()).orientation(), 0, AI::Flags::MOVE_RAM_BALL, AI::Flags::PRIO_HIGH);
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::ram(World &world) {
	const Tactic::Ptr p(new Ram(world));
	return p;
}

