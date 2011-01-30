#include "ai/hl/stp/tactic/block.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Evaluate::EnemyRole;

namespace {
	class Block : public Tactic {
		public:
			Block(World &world, EnemyRole::Ptr enemy) : Tactic(world), enemy(enemy) {
			}
		private:
			EnemyRole::Ptr enemy;
			double score(Player::Ptr player) const {
				return 1.0 / (1.0 + (player->position() - enemy->evaluate()->position()).len());
			}
			void execute() {
				player->move(enemy->evaluate()->position(), (world.ball().position() - player->position()).orientation(), param.move_flags, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_HIGH);
			}
	};
}

Tactic::Ptr AI::HL::STP::Tactic::block(World &world, EnemyRole::Ptr enemy) {
	const Tactic::Ptr p(new Block(world, enemy));
	return p;
}

