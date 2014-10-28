#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/action/block.h"
#include "ai/hl/stp/action/stop.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Enemy;
namespace Action = AI::HL::STP::Action;

namespace {
	// should take into account of enemy velocity etc

	class BlockGoal final : public Tactic {
		public:
			explicit BlockGoal(World world, Enemy::Ptr enemy) : Tactic(world), enemy(enemy) {
			}

		private:
			Enemy::Ptr enemy;
			Player select(const std::set<Player> &players) const override;
			void execute() override;
			Glib::ustring description() const override {
				return u8"block-goal";
			}
	};

	Player BlockGoal::select(const std::set<Player> &players) const {
		if (!enemy->evaluate()) {
			return *(players.begin());
		}
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(enemy->evaluate().position()));
	}

	void BlockGoal::execute() {
		if (!enemy->evaluate()) {
			Action::stop(world, player);
//			player.dribble_stop();
			return;
		}
		AI::HL::STP::Action::block_goal(world, player, enemy->evaluate());
	}

	class BlockBall final : public Tactic {
		public:
			BlockBall(World world, Enemy::Ptr enemy) : Tactic(world), enemy(enemy) {
			}

		private:
			Enemy::Ptr enemy;
			Player select(const std::set<Player> &players) const override;
			void execute() override;
			Glib::ustring description() const override {
				return u8"block-ball";
			}
	};

	Player BlockBall::select(const std::set<Player> &players) const {
		if (!enemy->evaluate()) {
			return *(players.begin());
		}
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(enemy->evaluate().position()));
	}

	void BlockBall::execute() {
		if (!enemy->evaluate()) {
			Action::stop(world, player);
			player.dribble(AI::BE::Player::DribbleMode::STOP);
			return;
		}

		AI::HL::STP::Action::block_ball(world, player, enemy->evaluate());
	}
}

Tactic::Ptr AI::HL::STP::Tactic::block_goal(World world, Enemy::Ptr enemy) {
	Tactic::Ptr p(new BlockGoal(world, enemy));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::block_ball(World world, Enemy::Ptr enemy) {
	Tactic::Ptr p(new BlockBall(world, enemy));
	return p;
}

