#include "ai/hl/stp/tactic/block.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Enemy;

namespace {
	class Block : public Tactic {
		public:
			Block(World &world, Enemy::Ptr enemy) : Tactic(world), enemy(enemy) {
			}
		private:
			Enemy::Ptr enemy;
			double score(Player::Ptr player) const {
				if (!enemy->evaluate().is()) {
					return 0;
				}
				return -(player->position() - enemy->evaluate()->position()).lensq();
			}
			void execute() {
				if (!enemy->evaluate().is()) {
					// do nothing??
					player->move(player->position(), player->orientation(), param.move_flags, AI::Flags::MOVE_NORMAL, param.move_priority);
					return;
				}

				Point nearEnemy(enemy->evaluate()->position().x - Robot::MAX_RADIUS * 3, enemy->evaluate()->position().y);
				player->move(nearEnemy, (world.ball().position() - player->position()).orientation(), param.move_flags, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::block(World &world, Enemy::Ptr enemy) {
	const Tactic::Ptr p(new Block(world, enemy));
	return p;
}

