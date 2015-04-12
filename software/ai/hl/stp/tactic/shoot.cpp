#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/action/intercept.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/player.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/stp/action/move.h"
#include "geom/util.h"
#include "geom/angle.h"

using AI::HL::STP::BALL_MAX_SPEED;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::STP::Action;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;
namespace Evaluation = AI::HL::STP::Evaluation;

namespace {
	BoolParam new_shoot(u8"Enable autokick threshold", u8"AI/HL/STP/Tactic/Shoot", true);
	DegreeParam shoot_thresh(u8"Threshold (degrees)", u8"AI/HL/STP/Tactic/Shoot", 5.0, 0.0, 90.0);

	class ShootGoal final : public Tactic {
		public:
			explicit ShootGoal(World world, bool force) : Tactic(world, true), kick_attempted(false), force(force), shoot_score(Angle::zero())
			{
				// world.friendly_team().signal_robot_removing().connect(sigc::mem_fun(this, &ShootGoal::on_player_removed));
			}

		private:
			bool kick_attempted;
			bool force;
			Angle shoot_score;
			// mutable Player shooter;

			// void on_player_removed(std::size_t index) {
			// if(passer.is() && world.friendly_team().get(index) == Player::CPtr(passer)){
			// passer.reset();
			// }
			// }



			bool done() const override;
			Player select(const std::set<Player> &players) const override;
			void execute() override;
			void player_changed() override;
			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const override;
			Glib::ustring description() const override {
				return u8"shoot-goal";
			}

			std::vector<Point> obstacles;
			Point target;
	};

	class ShootTarget final : public Tactic {
		public:
			explicit ShootTarget(World world, const Coordinate target) : Tactic(world, true), target(target), kick_attempted(false) {
			}

		private:
			Coordinate target;
			bool kick_attempted;
			bool done() const override;
			Player select(const std::set<Player> &players) const override;
			void execute() override;
			void player_changed() override;
			Glib::ustring description() const override {
				return u8"shoot-target";
			}
	};

	bool ShootGoal::done() const {
		return player /* && kick_attempted*/ && player.autokick_fired();
	}

	Player ShootGoal::select(const std::set<Player> &players) const {
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
		obstacles.clear();
		
		for(auto i : world.enemy_team())
			obstacles.push_back(i.position());
		for(auto i : world.friendly_team())
			if((i.position() - player.position()).len() > 0.1)
				obstacles.push_back(i.position());

		target = angle_sweep_circles(player.position(), world.field().enemy_goal_boundary().first, world.field().enemy_goal_boundary().second, obstacles, Robot::MAX_RADIUS).first;

		shoot_goal(world, player);
		
		/* don't know what this does
		if (AI::HL::STP::Action::shoot_target(world, player, target, BALL_MAX_SPEED)) {
			kick_attempted = true;
		}
		Angle cur_shoot_score = AI::HL::STP::Evaluation::get_shoot_score(world, player);
		if (new_shoot && ((cur_shoot_score < shoot.target);
 			return;
 		}

_score + Angle::of_radians(1e-9) && shoot_score > Angle::zero()) || cur_shoot_score > shoot_thresh)) {
			player.autokick(BALL_MAX_SPEED);
		}
		shoot_score = cur_shoot_score;
		*/
	}

	void ShootGoal::draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const {
		ctx->set_source_rgb(1.0, 0.2, 0.2);
		ctx->set_line_width(0.02);
		ctx->arc(target.x, target.y, 0.02, 0, 2*M_PI);
		ctx->fill();
	}

	bool ShootTarget::done() const {
		return player /* && kick_attempted */ && player.autokick_fired();
	}

	Player ShootTarget::select(const std::set<Player> &players) const {
		// if a player attempted to shoot, keep the player
		if (players.count(player) && Evaluation::possess_ball(world, player)) {
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

