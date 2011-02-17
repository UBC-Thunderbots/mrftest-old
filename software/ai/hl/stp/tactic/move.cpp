#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/util.h"
#include <algorithm>

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;

namespace {
	class Move : public Tactic {
		public:
			Move(const World &world, const Coordinate dest) : Tactic(world), dest(dest) {
			}

		private:
			const Coordinate dest;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
	};

	Player::Ptr Move::select(const std::set<Player::Ptr> &players) const {
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(dest()));
	}

	void Move::execute() {
		player->move(dest(), (world.ball().position() - player->position()).orientation(), param.move_flags, AI::Flags::MOVE_NORMAL, param.move_priority);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::move(const World &world, const Coordinate dest) {
	const Tactic::Ptr p(new Move(world, dest));
	return p;
}

