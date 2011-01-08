#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	class Move : public Tactic {
		public:
			Move(World &world, const Point dest) : Tactic(world), dest(dest) {
			}

		private:
			const Point dest;

			bool done() const {
				return (player->position() - dest).len() < AI::HL::Util::POS_CLOSE;
			}

			double score(Player::Ptr player) const {
				return 1.0 / (1.0 + (player->position() - dest).len());
			}

			void execute() {
				// TODO: flags
				player->move(dest, (world.ball().position() - player->position()).orientation(), 0, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::move(World &world, const Point dest) {
	const Tactic::Ptr p(new Move(world, dest));
	return p;
}

