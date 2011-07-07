#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/player.h"
#include "ai/hl/stp/param.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;
namespace Evaluation = AI::HL::STP::Evaluation;

namespace {
	class ShootGoal : public Tactic {
		public:
			ShootGoal(const World &world, bool force) : Tactic(world, true), kick_attempted(false), force(force) {
			}

		private:
			bool kick_attempted;
			bool force;

			bool done() const;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			void player_changed();
			std::string description() const {
				return "shoot-goal";
			}
	};

	class ShootTarget : public Tactic {
		public:
			ShootTarget(const World &world, const Coordinate target) : Tactic(world, true), target(target), kick_attempted(false) {
			}

		private:
			Coordinate target;
			bool kick_attempted;
			bool done() const;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			void player_changed();
			std::string description() const {
				return "shoot-target";
			}
	};

	bool ShootGoal::done() const {
		return player.is() && kick_attempted && player->autokick_fired();
	}

	Player::Ptr ShootGoal::select(const std::set<Player::Ptr> &players) const {
		// if a player attempted to shoot, keep the player
		if (kick_attempted && players.count(player)) {
			return player;
		}
		return select_baller(world, players);
	}

	void ShootGoal::player_changed() {
		kick_attempted = false;
	}

	void ShootGoal::execute() {
		if (AI::HL::STP::Action::shoot_goal(world, player, force)) {
			kick_attempted = true;
		}
		player->flags(0);
	}

	bool ShootTarget::done() const {
		return player.is() && kick_attempted && player->autokick_fired();
	}

	Player::Ptr ShootTarget::select(const std::set<Player::Ptr> &players) const {
		// if a player attempted to shoot, keep the player
		Player::CPtr player_c = player;
		if (players.count(player) && Evaluation::possess_ball(world, player_c)) {
			return player;
		}
		if (kick_attempted && players.count(player)) {
			return player;
		}
		return select_baller(world, players);
	}

	void ShootTarget::player_changed() {
		kick_attempted = false;
	}

	void ShootTarget::execute() {
		if (AI::HL::STP::Action::shoot_target(world, player, target.position(), AI::HL::STP::Action::pass_speed)) {
			kick_attempted = true;
		}
		player->flags(0);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::shoot_goal(const World &world, bool force) {
	const Tactic::Ptr p(new ShootGoal(world, force));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::shoot_target(const World &world, const Coordinate target) {
	const Tactic::Ptr p(new ShootTarget(world, target));
	return p;
}

