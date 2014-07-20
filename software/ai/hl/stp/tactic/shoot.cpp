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
	BoolParam new_shoot(u8"ShootGoal: enable autokick threshold", u8"AI/STP/Tactic/shoot", true);
	DegreeParam shoot_thresh(u8"ShootGoal: threshold (degrees)", u8"AI/STP/Tactic/shoot", 5.0, 0.0, 90.0);

	class ShootGoal : public Tactic {
		public:
			ShootGoal(World world, bool force) : Tactic(world, true), kick_attempted(false), force(force), shoot_score(Angle::zero()),
			position_reset(true), best_score(std::make_pair(Point(0, 0), std::make_pair(Point(0, 0), Angle::zero())))
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



			bool done() const;
			Player select(const std::set<Player> &players) const;
			void execute();
			void player_changed();
			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const;
			Glib::ustring description() const {
				return u8"shoot-goal";
			}

			bool position_reset;
			Point enemy_goal_positive, enemy_goal_negative;
			Point initial_position;
			Point target;
			std::vector<std::pair<Point, std::pair<Point, Angle>>> scores;
			std::pair<Point, std::pair<Point, Angle>> best_score;
			std::vector<Point> obstacles;
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
		initial_position = player.position();
		kick_attempted = false;
	}

	void ShootGoal::execute() {
		best_score = std::make_pair(Point(0,0), std::make_pair(Point(0,0), Angle::zero()));
		if((world.ball().position() - player.position()).len() > Robot::MAX_RADIUS + 0.05)
			position_reset = true;
		if(position_reset) {
			initial_position = player.position();
			position_reset = false;
		}

		scores.clear();
		obstacles.clear();
		scores.clear();
		
		//first: find the best location to shoot at in a circle centered at the baller with a radius of 0.5m
		enemy_goal_positive = world.field().enemy_goal_boundary().first.x > 0.0 ? 
			world.field().enemy_goal_boundary().first : world.field().enemy_goal_boundary().second;
		enemy_goal_negative = world.field().enemy_goal_boundary().first.x < 0.0 ?
			world.field().enemy_goal_boundary().first : world.field().enemy_goal_boundary().second;

		for(auto i : world.enemy_team())
			obstacles.push_back(i.position());
		for(auto i : world.friendly_team())
			if((i.position() - player.position()).len() > 0.1)
				obstacles.push_back(i.position());

		for(double x = initial_position.x - 0.50; x < initial_position.x + 0.50; x += 0.04) {
			for(double y = initial_position.y - 0.50; y < initial_position.y + 0.50; y += 0.04) {
				if((Point(x, y) - initial_position).len() < 0.45) {
					scores.push_back(
							std::make_pair(
								Point(x, y),
								angle_sweep_circles(
									Point(x, y),
									enemy_goal_positive,
									enemy_goal_negative,
									obstacles,
									Robot::MAX_RADIUS
									)
								)
							);
				}
			}
		}

		if(!Evaluation::possess_ball(world, player)) {
			/*
			player.dribble_slow();
			Point p = player.position(), b = world.ball().position();
			if(line_point_dist(p, b, b - Point(1, 0)) > 0.1 || p.x > b.x)
				move(player, Angle::zero(),
						world.ball().position() - Point(Robot::MAX_RADIUS + 0.07, 0));
			else
				move(player, Angle::zero(), 
						world.ball().position());
 			return;
			*/
			intercept(player, world.ball().position());
			return;
 		}

		for(auto &i : scores) {
			for(auto p : world.enemy_team()) {
				if((p.position() - i.first).len() < Robot::MAX_RADIUS * 2 + 0.02)
					i.second.second = Angle::zero();
			}
			if(i.second.second > best_score.second.second)
				best_score = i;
		}

		target = angle_sweep_circles(
			best_score.first, 
			world.field().enemy_goal_boundary().first, 
			world.field().enemy_goal_boundary().second, 
			obstacles, 
			Robot::MAX_RADIUS
			).first;
		
		if((best_score.first - player.position()).len() > 0.03 || (player.orientation() - (best_score.second.first - player.position()).orientation()).to_degrees() > 5) {
			move(player, (best_score.second.first - player.position()).orientation(), best_score.first);
			player.flags(AI::Flags::FLAG_CAREFUL);
			player.type(AI::Flags::MoveType::PIVOT);
			return;
		}

		shoot_target(world, player, target, BALL_MAX_SPEED);

		/* don't know what this does
		if (AI::HL::STP::Action::shoot_target(world, player, target, BALL_MAX_SPEED)) {
			kick_attempted = true;
		}
		Angle cur_shoot_score = AI::HL::STP::Evaluation::get_shoot_score(world, player);
		if (new_shoot && ((cur_shoot_score < shoot.target);
 			return;
 		}

		//first: find the best location to shoot at in a circle centered at the baller with a radius of 0.5m
		enemy_goal_positive = world.field().enemy_goal_boundary().first.x > 0.0 ? 
			world.field().enemy_goal_boundary().first : world.field().enemy_goal_boundary().second;
		enemy_goal_negative = world.field().enemy_goal_boundary().first.x < 0.0 ?
_score + Angle::of_radians(1e-9) && shoot_score > Angle::zero()) || cur_shoot_score > shoot_thresh)) {
			player.autokick(BALL_MAX_SPEED);
		}
		shoot_score = cur_shoot_score;
		*/
	}

	void ShootGoal::draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const {
		ctx->set_line_width(0.025);
		for(auto i : scores) {
			ctx->set_source_rgba(i.second.second.to_degrees() / 15, i.second.second.to_degrees() / 15, i.second.second.to_degrees() / 15, 0.5);
			ctx->arc(i.first.x, i.first.y, 0.01, 0.0, 2*M_PI);
			ctx->fill();
		}
		ctx->set_source_rgba(1.0, 0.0, 0.0, 1.0);
		ctx->arc(best_score.first.x, best_score.first.y, 0.02, 0.0, 2*M_PI);
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

