#include "ai/hl/stp/tactic/block.h"

#include "ai/hl/stp/action/block.h"
#include "ai/hl/stp/action/stop.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/action/move.h"

//#include "ai/hl/stp/tactic/tactic.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Enemy;
namespace Action = AI::HL::STP::Action;

namespace AI {
	namespace HL {
		namespace STP {
			namespace Action {
				extern DoubleParam block_threshold;
				extern DoubleParam block_angle;
			}
		}
	}
}

namespace {
	// should take into account of enemy velocity etc
	class BlockGoal final : public Tactic {
		public:
			explicit BlockGoal(World world, Enemy::Ptr enemy) : Tactic(world), enemy(enemy) {
			}

		private:
			Enemy::Ptr enemy;
			Player select(const std::set<Player> &players) const override;
			void execute(caller_t& ca) override;
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

	void BlockGoal::execute(caller_t& ca) {
		while(true) {
			if (!enemy->evaluate()) {
				Action::stop(ca, world, player());
				yield(ca);
				continue;
	//			player.dribble_stop();
			}
			AI::HL::STP::Action::block_goal(ca, world, player(), enemy->evaluate());
			Point dirToGoal = (world.field().friendly_goal() - player().position().norm());
			Point target = player().position() + (AI::HL::STP::Action::block_threshold * Robot::MAX_RADIUS * dirToGoal);
			Action::move(ca, world, player(), target);
			yield(ca);
		}
	}

	class BlockBall final : public Tactic {
		public:
			BlockBall(World world, Enemy::Ptr enemy) : Tactic(world), enemy(enemy) {
			}

		private:
			Enemy::Ptr enemy;
			Player select(const std::set<Player> &players) const override;
			void execute(caller_t& ca) override;
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

	void BlockBall::execute(caller_t& ca) {
		while(true) {
			if (!enemy->evaluate()) {
				Action::stop(ca, world, player());
				yield(ca);
				continue;
			}
			Point dirToBall = (world.ball().position() - player().position()).norm();
			Point target = player().position() + (AI::HL::STP::Action::block_threshold * Robot::MAX_RADIUS * dirToBall);
			Action::move(ca, world, player(), target);
			yield(ca);
		}
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

