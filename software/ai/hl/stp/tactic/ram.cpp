#include "ai/hl/stp/tactic/ram.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
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
				return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(world.ball().position()));
			}

			void execute() {
				
				player->move(world.ball().position(), (world.ball().position() - player->position()).orientation(), Point());
				player->type(AI::Flags::MoveType::RAM_BALL);
				player->prio(AI::Flags::MovePrio::HIGH);
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::ram(const World &world) {
	const Tactic::Ptr p(new Ram(world));
	return p;
}

