#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/player.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/param.h"
#include "geom/util.h"
#include "geom/angle.h"

using AI::HL::STP::BALL_MAX_SPEED;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;
namespace Evaluation = AI::HL::STP::Evaluation;

namespace {
	BoolParam new_shoot(u8"ShootGoal: enable autokick threshold", u8"AI/STP/Tactic/shoot", true);
	DegreeParam shoot_thresh(u8"ShootGoal: threshold (degrees)", u8"AI/STP/Tactic/shoot", 5.0, 0.0, 90.0);

	class ShootGoal : public Tactic {
		public:
			ShootGoal(World world, bool force) : Tactic(world, true), kick_attempted(false), force(force), shoot_score(Angle::zero()) {
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



			bool done() const;
			Point target;
			Player select(const std::set<Player> &players) const;
			void execute();
			void player_changed();
			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const;
			Glib::ustring description() const {
				return u8"shoot-goal";
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
			Player select(const std::set<Player> &players) const;
			void execute();
			void player_changed();
			Glib::ustring description() const {
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
		std::vector<Point> positions; 

		for(auto i : world.enemy_team())
			positions.push_back(i.position());
		for(auto i : world.friendly_team())
			if((i.position() - player.position()).len() > 0.1)
				positions.push_back(i.position());

		target = angle_sweep_circles(player.position(), world.field().enemy_goal_boundary().first, world.field().enemy_goal_boundary().second, positions, Robot::MAX_RADIUS).first;

		if (AI::HL::STP::Action::shoot_target(world, player, target, BALL_MAX_SPEED)) {
			kick_attempted = true;
		}
		Angle cur_shoot_score = AI::HL::STP::Evaluation::get_shoot_score(world, player);
		if (new_shoot && ((cur_shoot_score < shoot_score + Angle::of_radians(1e-9) && shoot_score > Angle::zero()) || cur_shoot_score > shoot_thresh)) {
			player.autokick(BALL_MAX_SPEED);
		}
		shoot_score = cur_shoot_score;
	}

	void ShootGoal::draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const {
		if((target - Point(0,0)).len() < 0.2)
			return;

		ctx->set_source_rgba(1.0, 0.0, 0.0, 1.0);
		ctx->set_line_width(0.02);

		ctx->move_to(player.position().x, player.position().y);
		ctx->line_to(target.x, target.y);
		ctx->stroke();
	}

	bool ShootTarget::done() const {
		return player /* && kick_attempted */ && player.autokick_fired();
	}

	Player ShootTarget::select(const std::set<Player> &players) const {
		// if a player attempted to shoot, keep the player
		Player player_c = player;
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

