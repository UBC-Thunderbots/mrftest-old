#include "ai/hl/stp/tactic/move_penalty.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;

namespace {
	class MovePenalty : public Tactic {
		public:
			MovePenalty(World &world, const Coordinate dest) : Tactic(world, true), dest(dest) {
			}

		private:
			const Coordinate dest;
			bool done() const;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
	};

	bool MovePenalty::done() const {
		return (player->position() - dest()).len() < AI::HL::Util::POS_CLOSE;
	}

	Player::Ptr MovePenalty::select(const std::set<Player::Ptr> &players) const {
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(dest()));
	}

	void MovePenalty::execute() {
		player->move(dest(), (world.ball().position() - player->position()).orientation(), 0, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_HIGH);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::move_penalty(World &world, const Coordinate dest) {
	const Tactic::Ptr p(new MovePenalty(world, dest));
	return p;
}

