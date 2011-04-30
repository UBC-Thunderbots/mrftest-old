#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/action/block.h"
#include "ai/hl/stp/action/stop.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Enemy;
namespace Action = AI::HL::STP::Action;

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
#warning take into account of enemy velocity etc
			Action::stop(world, player);
			return;
		}
		
		AI::HL::STP::Action::block(world, player, enemy->evaluate());
		
		// TODO: think of when and how player should block enemy robot from passing
	}
}

Tactic::Ptr AI::HL::STP::Tactic::block(const World &world, Enemy::Ptr enemy) {
	const Tactic::Ptr p(new Block(world, enemy));
	return p;
}

