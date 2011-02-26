#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/action/actions.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;

namespace {
	class Shoot : public Tactic {
		public:
			Shoot(const World &world) : Tactic(world, true) {
			}

		private:
			bool done() const;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
	};

	class ShootTarget : public Tactic {
		public:
			ShootTarget(const World &world, const Coordinate target) : Tactic(world, true), target(target) {
			}

		private:
			Coordinate target;
			bool done() const;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
	};

	bool Shoot::done() const {
#warning TODO
		return !player->has_ball();
	}

	Player::Ptr Shoot::select(const std::set<Player::Ptr> &players) const {
		for (auto it = players.begin(); it != players.end(); ++it) {
			if ((*it)->has_ball()) {
				return *it;
			}
		}
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(world.ball().position()));
	}

	void Shoot::execute() {
		//set_ssm(AI::HL::STP::SSM::move_ball());
		AI::HL::STP::Actions::shoot(world, player, 0);
	}

	bool ShootTarget::done() const {
#warning TODO
		return !player->has_ball();
	}

	Player::Ptr ShootTarget::select(const std::set<Player::Ptr> &players) const {
		for (auto it = players.begin(); it != players.end(); ++it) {
			if ((*it)->has_ball()) {
				return *it;
			}
		}
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(world.ball().position()));
	}

	void ShootTarget::execute() {
		//set_ssm(AI::HL::STP::SSM::move_ball());
		AI::HL::STP::Actions::shoot(world, player, 0, target());
	}

}

Tactic::Ptr AI::HL::STP::Tactic::shoot(const World &world) {
	const Tactic::Ptr p(new Shoot(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::shoot(const World &world, const Coordinate target) {
	const Tactic::Ptr p(new ShootTarget(world, target));
	return p;
}

