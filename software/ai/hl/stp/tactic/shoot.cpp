#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;

namespace {
	class Shoot : public Tactic {
		public:
			Shoot(const World &world) : Tactic(world, true), has_shot(false) {
			}

		private:
			bool has_shot;
			bool done() const;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
	};

	class ShootTarget : public Tactic {
		public:
			ShootTarget(const World &world, const Coordinate target) : Tactic(world, true), target(target), has_shot(false) {
			}

		private:
			Coordinate target;
			bool has_shot;
			bool done() const;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
	};

	bool Shoot::done() const {
#warning tactic should control shooting
		return has_shot;
	}

	Player::Ptr Shoot::select(const std::set<Player::Ptr> &players) const {
		return select_baller(world, players);
	}

	void Shoot::execute() {
		//set_ssm(AI::HL::STP::SSM::move_ball());
		if (AI::HL::STP::Action::shoot(world, player)) {
			has_shot = true;
		}
	}

	bool ShootTarget::done() const {
#warning tactic should control shooting
		return has_shot;
	}

	Player::Ptr ShootTarget::select(const std::set<Player::Ptr> &players) const {
		return select_baller(world, players);
	}

	void ShootTarget::execute() {
		//set_ssm(AI::HL::STP::SSM::move_ball());
		if (AI::HL::STP::Action::shoot(world, player, target())) {
			has_shot = true;
		}
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

