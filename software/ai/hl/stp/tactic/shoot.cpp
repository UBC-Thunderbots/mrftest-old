#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/tactic/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;

namespace {
	class ShootGoal : public Tactic {
		public:
			ShootGoal(const World &world) : Tactic(world, true), has_shot(false) {
			}

		private:
			bool has_shot;
			bool done() const;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			std::string description() const {
				return "shoot-goal";
			}
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
			std::string description() const {
				return "shoot-target";
			}
	};

	bool ShootGoal::done() const {
#warning tactic should control shooting
		return has_shot;
	}

	Player::Ptr ShootGoal::select(const std::set<Player::Ptr> &players) const {
		return select_baller(world, players);
	}

	void ShootGoal::execute() {
		if (AI::HL::STP::Action::shoot_goal(world, player)) {
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
		if (AI::HL::STP::Action::shoot_target(world, player, target.position(), false)) {
			has_shot = true;
		}
	}
}

Tactic::Ptr AI::HL::STP::Tactic::shoot_goal(const World &world) {
	const Tactic::Ptr p(new ShootGoal(world));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::shoot_target(const World &world, const Coordinate target) {
	const Tactic::Ptr p(new ShootTarget(world, target));
	return p;
}

