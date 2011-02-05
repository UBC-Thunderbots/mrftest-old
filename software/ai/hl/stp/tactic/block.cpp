#include "ai/hl/stp/tactic/block.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Evaluation::EnemyRole;

namespace {
	class Block : public Tactic {
		public:
			Block(World &world, EnemyRole::Ptr enemy) : Tactic(world), enemy(enemy) {
			}
		private:
			EnemyRole::Ptr enemy;
			double score(Player::Ptr player) const {
				if (!enemy->exist()) {
					return 0;
				}
				return -(player->position() - enemy->evaluate()->position()).lensq();
			}
			void execute() {
				if (!enemy->exist()) {
					// do nothing??
					player->move(player->position(), player->orientation(), param.move_flags, AI::Flags::MOVE_NORMAL, param.move_priority);
					return;
				}

				Point nearEnemy(enemy->evaluate()->position().x - Robot::MAX_RADIUS * 3, enemy->evaluate()->position().y);
				player->move(nearEnemy, (world.ball().position() - player->position()).orientation(), param.move_flags, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::block(World &world, EnemyRole::Ptr enemy) {
	const Tactic::Ptr p(new Block(world, enemy));
	return p;
}

