#include "ai/hl/stp/tactic/cshoot.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/action/cshoot.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/action/intercept.h"
#include "ai/hl/stp/action/pivot.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/player.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/param.h"
#include "ai/hl/stp/action/move.h"
#include "geom/util.h"
#include "geom/angle.h"
#include <iostream>

using AI::HL::STP::BALL_MAX_SPEED;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::STP::Action;
using namespace AI::HL::W;
using AI::HL::STP::Coordinate;
namespace Evaluation = AI::HL::STP::Evaluation;

namespace {
	class CShootGoal : public Tactic {
		public:
			CShootGoal(World world, bool force) : Tactic(world, true), kick_attempted(false), force(force),
			position_reset(true), timer(0), best_score(std::make_tuple(Point(0, 0), Point(0, 0), 0))
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
				return u8"cshoot-goal";
			}

			bool position_reset;
			int timer;
			Point enemy_goal_positive, enemy_goal_negative;
			Point initial_position;
			Point target;
			std::vector<std::tuple<Point, Point, double>> scores;
			std::tuple<Point, Point, double> best_score;
			std::vector<Point> obstacles;
	};

	bool CShootGoal::done() const {
		return false;
		//return player /* && kick_attempted*/ && player.autokick_fired();
	}

	Player CShootGoal::select(const std::set<Player> &players) const {
		// if a player attempted to shoot, keep the player
		if (kick_attempted && players.count(player)) {
			return player;
		}
		return select_baller(world, players, player);
	}

	void CShootGoal::player_changed() {
		initial_position = player.position();
		kick_attempted = false;
	}

	void CShootGoal::execute() {
		if(timer >= 0 && !player.has_ball()) {
			timer--;
		}
		else if(timer <= 10 && player.has_ball()) {
			timer++;
		}

		best_score = std::make_tuple(Point(0,0), Point(0,0), 0.0);

		if((world.ball().position() - player.position()).len() > Robot::MAX_RADIUS + 0.02)
			position_reset = true;

		if(position_reset) {
			initial_position = player.position();
			position_reset = false;
		}

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
					std::pair<Point, Angle> sweep_score = angle_sweep_circles(
							Point(x, y),
								enemy_goal_positive,
								enemy_goal_negative,
								obstacles,
								Robot::MAX_RADIUS
								);
					double shoot_score = sweep_score.second.to_degrees();

					for(auto p : world.enemy_team()) {
						if((p.position() - Point(x, y)).len() < Robot::MAX_RADIUS * 2 + 0.03)
							shoot_score = 0.0;
					}

					if((Point(x, y) - world.field().enemy_goal()).len() < 0.3)
						shoot_score -= (0.3 - (Point(x, y) - world.field().enemy_goal()).len())*40;

					shoot_score += (8 - (Point(x, y) - player.position()).len() * 8);

					scores.push_back(std::make_tuple(Point(x, y), sweep_score.first, shoot_score));
				}
			}
		}

		/* if(!player.has_ball()) {
			player.dribble_slow();
			Point p = player.position(), b = world.ball().position();
			if(line_point_dist(p, b, b - Point(1, 0)) > 0.1 || p.x > b.x)
				move(player, Angle::zero(),
						world.ball().position() - Point(Robot::MAX_RADIUS + 0.07, 0));
			else
				move(player, Angle::zero(), 
						world.ball().position());
 			return;
			if(timer < 0) {
				intercept(player, world.ball().position());
				return;
			}
 		} */

		for(auto &i : scores) {
			if(std::get<2>(i) > std::get<2>(best_score))
				best_score = i;
		}

		target = angle_sweep_circles(
			std::get<0>(best_score), 
			world.field().enemy_goal_boundary().first, 
			world.field().enemy_goal_boundary().second, 
			obstacles, 
			Robot::MAX_RADIUS
			).first;

		if(!player.has_ball()) {
			intercept(player, target);
			return;
		}

		if(timer < 10) return;

		/*
		if(((std::get<0>best_score - player.position()).len() > 0.05) || 
				((player.orientation() - (std::get<1>best_score - player.position()).orientation()).to_degrees() > 2.0)) {
			move(player, Angle::of_degrees((player.orientation().to_degrees() - (std::get<1>best_score - player.position()).orientation().to_degrees() > 0 ?
					-1 : 1) * 6) + player.orientation(), 
					player.position() + (std::get<0>best_score - player.position()).norm(0.2));
			//player.flags(AI::Flags::FLAG_CAREFUL);
			return;
		}
		*/

		if(((std::get<0>(best_score) - player.position()).len() > 0.05) || 
				((player.orientation() - (std::get<1>(best_score) - player.position()).orientation()).to_degrees() > 2.0)) {
			player.dribble_slow();
			move(player, (std::get<1>(best_score) - player.position()).orientation(), std::get<0>(best_score), Point(0.01, 0.01));
			return;
		}

		player.autokick(BALL_MAX_SPEED);
	}

	void CShootGoal::draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const {
		ctx->set_line_width(0.025);
		for(auto i : scores) {
			ctx->set_source_rgba(std::get<2>(i) / 15, std::get<2>(i) / 15, std::get<2>(i) / 15, 0.5);
			ctx->arc(std::get<0>(i).x, std::get<0>(i).y, 0.02, 0.0, 2*M_PI);
			ctx->fill();
		}
		ctx->set_source_rgba(1.0, 0.0, 0.0, 1.0);
		ctx->arc(std::get<0>(best_score).x, std::get<0>(best_score).y, 0.02, 0.0, 2*M_PI);
		
		ctx->fill();

		ctx->set_source_rgba(1.0, 0.7, 0.7, 0.8);
		ctx->move_to(std::get<0>(best_score).x, std::get<0>(best_score).y);
		ctx->line_to(std::get<1>(best_score).x, std::get<1>(best_score).y);
		ctx->stroke();
	}
}

Tactic::Ptr AI::HL::STP::Tactic::cshoot_goal(World world, bool force) {
	Tactic::Ptr p(new CShootGoal(world, force));
	return p;
}

