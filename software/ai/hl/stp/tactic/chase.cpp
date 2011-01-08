#include "ai/hl/stp/tactic/chase.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	class Chase : public Tactic {
		public:
			Chase(World &world) : Tactic(world) {
			}

		private:
			bool done_;

			bool done() const {
				return done_;
			}

			void player_changed() {
				done_ = (player->has_ball());
			}

			double score(Player::Ptr player) const {
				if (player->has_ball()) return 1e99;
				return -(player->position() - world.ball().position()).lensq();
			}

			void execute() {
				// if it has the ball, stay there
				if (player->has_ball()) {
					done_ = true;
					player->move(player->position(), player->orientation(), 0, AI::Flags::MOVE_DRIBBLE, AI::Flags::PRIO_HIGH);
					return;
				}

				player->move(world.ball().position(), (world.ball().position() - player->position()).orientation(), move_flags, AI::Flags::MOVE_CATCH, AI::Flags::PRIO_HIGH);
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::chase(World &world) {
	const Tactic::Ptr p(new Chase(world));
	return p;
}

