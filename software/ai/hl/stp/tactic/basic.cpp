#include "ai/hl/stp/tactic/basic.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

// block

namespace {
	class Block : public Tactic {
		public:
			Block(World &world, Robot::Ptr robot) : Tactic(world), robot(robot) {
			}

		private:
			Robot::Ptr robot;

			double score(Player::Ptr player) const {
				return 1.0 / (1.0 + (player->position() - robot->position()).len());
			}

			void execute() {
				player->move(robot->position(), (world.ball().position() - player->position()).orientation(), param.move_flags, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_HIGH);
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::block(World &world, Robot::Ptr robot) {
	const Tactic::Ptr p(new Block(world, robot));
	return p;
}

// idle

namespace {
	class Idle : public Tactic {
		public:
			Idle(World &world) : Tactic(world) {
			}

		private:
			double score(Player::Ptr) const {
				return 1;
			}
			void execute() {
				// nothing lol
				Point dest = Point(0, world.field().width() / 2);
				player->move(dest, (world.ball().position() - player->position()).orientation(), param.move_flags, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_LOW);
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::idle(World &world) {
	const Tactic::Ptr p(new Idle(world));
	return p;
}

