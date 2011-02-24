#include "ai/hl/stp/tactic/patrol.h"
#include "ai/hl/util.h"
#include <iostream>

using namespace AI::HL::W;
using namespace AI::HL::STP::Tactic;
using AI::HL::STP::Coordinate;

namespace {
#warning TODO: incomplete

	class Patrol : public Tactic {
		public:
			Patrol(AI::HL::W::World &world, Coordinate w1, Coordinate w2);

			void execute();

		protected:
			Coordinate p1, p2;
			bool goto_target1;
	};

	Patrol::Patrol(AI::HL::W::World &world, Coordinate w1, Coordinate w2) : Tactic(world), p1(w1), p2(w2) {
	}

	void Patrol::execute() {
		if (!player.is()) {
			return;
		}

		if ((player->position() - p1()).len() < AI::HL::Util::POS_CLOSE) {
			goto_target1 = false;
		} else if ((player->position() - p2()).len() < AI::HL::Util::POS_CLOSE) {
			goto_target1 = true;
		}

#warning TODO: fix flags

		if (goto_target1) {
			player->move(p1(), (world.ball().position() - player->position()).orientation(), 0, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
		} else {
			player->move(p2(), (world.ball().position() - player->position()).orientation(), 0, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
		}
	}
}

