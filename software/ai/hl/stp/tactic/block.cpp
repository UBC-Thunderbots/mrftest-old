#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Enemy;

namespace {
	class Block : public Tactic {
		public:
			Block(const World &world, Enemy::Ptr enemy) : Tactic(world), enemy(enemy) {
			}

		private:
			Enemy::Ptr enemy;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
	};

	Player::Ptr Block::select(const std::set<Player::Ptr> &players) const {
		if (!enemy->evaluate().is()) {
			return *(players.begin());
		}
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(enemy->evaluate()->position()));
	}

	void Block::execute() {
		if (!enemy->evaluate().is()) {
			// do nothing??
			player->move(player->position(), player->orientation(), AI::Flags::calc_flags(world.playtype()), AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
			return;
		}

		Point near_enemy(enemy->evaluate()->position().x - Robot::MAX_RADIUS * 3, enemy->evaluate()->position().y);
		player->move(near_enemy, (world.ball().position() - player->position()).orientation(), AI::Flags::calc_flags(world.playtype()), AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::block(const World &world, Enemy::Ptr enemy) {
	const Tactic::Ptr p(new Block(world, enemy));
	return p;
}

