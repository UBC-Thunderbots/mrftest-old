#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/player.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/param.h"
#include "geom/angle.h"

using AI::HL::STP::BALL_MAX_SPEED;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;
namespace Evaluation = AI::HL::STP::Evaluation;

namespace {
	BoolParam new_shoot("Jason's shoot code", "AI/STP/Tactic/shoot", true);
	DegreeParam shoot_thresh("Above this shoot score shoot (degrees)", "AI/STP/Tactic/shoot", 5.0, 0.0, 90.0);

	class ShootGoal : public Tactic {
		public:
			ShootGoal(World world, bool force) : Tactic(world, true), kick_attempted(false), force(force), shoot_score(Angle::ZERO) {
				// world.friendly_team().signal_robot_removing().connect(sigc::mem_fun(this, &ShootGoal::on_player_removed));
			}

		private:
			bool kick_attempted;
			bool force;
			Angle shoot_score;
			// mutable Player::Ptr shooter;

			// void on_player_removed(std::size_t index) {
			// if(passer.is() && world.friendly_team().get(index) == Player::CPtr(passer)){
			// passer.reset();
			// }
			// }



			bool done() const;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			void player_changed();
			Glib::ustring description() const {
				return "shoot-goal";
			}
	};

	class ShootTarget : public Tactic {
		public:
			ShootTarget(World world, const Coordinate target) : Tactic(world, true), target(target), kick_attempted(false) {
			}

		private:
			Coordinate target;
			bool kick_attempted;
			bool done() const;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
			void player_changed();
			Glib::ustring description() const {
				return "shoot-target";
			}
	};

	bool ShootGoal::done() const {
		return player /* && kick_attempted*/ && player->autokick_fired();
	}

	Player::Ptr ShootGoal::select(const std::set<Player::Ptr> &players) const {
		// if a player attempted to shoot, keep the player
		if (kick_attempted && players.count(player)) {
			return player;
		}
		return select_baller(world, players, player);
	}

	void ShootGoal::player_changed() {
		kick_attempted = false;
	}

	void ShootGoal::execute() {
		if (AI::HL::STP::Action::shoot_goal(world, player, force)) {
			kick_attempted = true;
		}
		Angle cur_shoot_score = AI::HL::STP::Evaluation::get_shoot_score(world, player);
		if (new_shoot && ((cur_shoot_score < shoot_score + Angle::of_radians(1e-9) && shoot_score > Angle::ZERO) || cur_shoot_score > shoot_thresh)) {
			player->autokick(BALL_MAX_SPEED);
		}
		shoot_score = cur_shoot_score;
	}

	bool ShootTarget::done() const {
		return player /* && kick_attempted */ && player->autokick_fired();
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
		return select_baller(world, players, player);
	}

	void ShootTarget::player_changed() {
		kick_attempted = false;
	}

	void ShootTarget::execute() {
		if (AI::HL::STP::Action::shoot_target(world, player, target.position(), AI::HL::STP::Action::pass_speed)) {
			kick_attempted = true;
		}
	}
}

Tactic::Ptr AI::HL::STP::Tactic::shoot_goal(World world, bool force) {
	Tactic::Ptr p(new ShootGoal(world, force));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::shoot_target(World world, const Coordinate target) {
	Tactic::Ptr p(new ShootTarget(world, target));
	return p;
}

