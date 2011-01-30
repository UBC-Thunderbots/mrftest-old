#include "ai/hl/stp/tactic/pass.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	class PasserReady : public Tactic {
		public:
			PasserReady(World &world, Point p, Point t) : Tactic(world), pos(p), target(t) {
			}
		private:
			Point pos, target;
			double score(Player::Ptr player) const {
				if (player->has_ball()) return 1.0;
				return 0;
			}
			void execute() {
				// TODO: fix this movement
				player->move(pos, (world.ball().position() - player->position()).orientation(), param.move_flags, AI::Flags::MOVE_DRIBBLE, AI::Flags::PRIO_MEDIUM);
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::passer_ready(World &world, Point pos, Point target) {
	const Tactic::Ptr p(new PasserReady(world, pos, target));
	return p;
}

namespace {
	class PasseeReady : public Tactic {
		public:
			// ACTIVE tactic!
			PasseeReady(World &world, Point p) : Tactic(world, true), pos(p) {
			}
		private:
			Point pos;
			bool done() const {
				return (player->position() - pos).len() < AI::HL::Util::POS_CLOSE;
			}
			double score(Player::Ptr player) const {
				if (player->has_ball()) return 1.0;
				return 0;
			}
			void execute() {
				player->move(pos, (world.ball().position() - player->position()).orientation(), param.move_flags, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_HIGH);
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::passee_ready(World &world, Point pos) {
	const Tactic::Ptr p(new PasseeReady(world, pos));
	return p;
}

