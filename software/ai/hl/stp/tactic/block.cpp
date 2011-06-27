#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/action/block.h"
#include "ai/hl/stp/action/stop.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Enemy;
namespace Action = AI::HL::STP::Action;

namespace {
	class BlockGoal : public Tactic {
		public:
			BlockGoal(const World &world, Enemy::Ptr enemy) : Tactic(world), enemy(enemy) {
			}

		private:
			Enemy::Ptr enemy;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			std::string description() const {
				return "block-goal";
			}
	};

	Player::Ptr BlockGoal::select(const std::set<Player::Ptr> &players) const {
		if (!enemy->evaluate().is()) {
			return *(players.begin());
		}
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(enemy->evaluate()->position()));
	}

	void BlockGoal::execute() {
		if (!enemy->evaluate().is()) {
#warning take into account of enemy velocity etc
			Action::stop(world, player);
			return;
		}

		AI::HL::STP::Action::block_goal(world, player, enemy->evaluate());
	}

	class BlockBall : public Tactic {
		public:
			BlockBall(const World &world, Enemy::Ptr enemy) : Tactic(world), enemy(enemy) {
			}

		private:
			Enemy::Ptr enemy;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			std::string description() const {
				return "block-ball";
			}
	};

	Player::Ptr BlockBall::select(const std::set<Player::Ptr> &players) const {
		if (!enemy->evaluate().is()) {
			return *(players.begin());
		}
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(enemy->evaluate()->position()));
	}

	void BlockBall::execute() {
		if (!enemy->evaluate().is()) {
#warning take into account of enemy velocity etc
			Action::stop(world, player);
			return;
		}

		AI::HL::STP::Action::block_ball(world, player, enemy->evaluate());
	}
}

Tactic::Ptr AI::HL::STP::Tactic::block_goal(const World &world, Enemy::Ptr enemy) {
	const Tactic::Ptr p(new BlockGoal(world, enemy));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::block_ball(const World &world, Enemy::Ptr enemy) {
	const Tactic::Ptr p(new BlockBall(world, enemy));
	return p;
}

