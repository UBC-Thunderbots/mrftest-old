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
			double score(Player::Ptr player) const;
			void execute();
	};

	bool MovePenalty::done() const {
		return (player->position() - dest()).len() < AI::HL::Util::POS_CLOSE;
	}

	double MovePenalty::score(Player::Ptr player) const {
		return -(player->position() - dest()).lensq();
	}

	void MovePenalty::execute() {
	
		player->move(dest(), (world.ball().position() - player->position()).orientation(), param.move_flags, AI::Flags::MOVE_NORMAL, param.move_priority);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::move_penalty(World &world, const Coordinate dest) {
	const Tactic::Ptr p(new MovePenalty(world, dest));
	return p;
}

