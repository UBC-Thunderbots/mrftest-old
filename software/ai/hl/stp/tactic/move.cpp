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
				return -(player->position() - dest).lensq();
			}

			void execute() {
				player->move(dest, (world.ball().position() - player->position()).orientation(), param.move_flags, AI::Flags::MOVE_NORMAL, param.move_priority);
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::move(World &world, const Point dest) {
	const Tactic::Ptr p(new Move(world, dest));
	return p;
}

