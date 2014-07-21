#include "ai/hl/stp/tactic/cshoot.h"
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
			CShootGoal(World world, bool force) : Tactic(world, true), kick_attempted(false), force(force), shoot_score(Angle::zero()),
			position_reset(true), timer(0), best_score(std::make_pair(Point(0, 0), std::make_pair(Point(0, 0), Angle::zero())))
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
			std::vector<std::pair<Point, std::pair<Point, Angle>>> scores;
			std::pair<Point, std::pair<Point, Angle>> best_score;
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

		best_score = std::make_pair(Point(0,0), std::make_pair(Point(0,0), Angle::zero()));

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
					scores[scores.size() - 1].second.second += Angle::of_degrees(1 - (Point(x, y) - player.position()).len());
					scores[scores.size() - 1].second.second += Angle::of_degrees(player.position().x - x);
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

		if(!player.has_ball()) {
			intercept_pivot(world, player, target);
			return;
		}

		if(timer < 10) return;

		if(((best_score.first - player.position()).len() > 0.05) || 
				((player.orientation() - (best_score.second.first - player.position()).orientation()).to_degrees() > 2.0)) {
			move(player, Angle::of_degrees((player.orientation().to_degrees() - (best_score.second.first - player.position()).orientation().to_degrees() > 0 ?
					-1 : 1) * 6) + player.orientation(), 
					player.position() + (best_score.first - player.position()).norm(0.2));
			//player.flags(AI::Flags::FLAG_CAREFUL);
			player.type(AI::Flags::MoveType::DRIBBLE);
			return;
		}

		/* if(((best_score.first - player.position()).len() > 0.05) || 
				((player.orientation() - (best_score.second.first - player.position()).orientation()).to_degrees() > 2.0)) {
			move(player, (best_score.second.first - player.position()).orientation(), best_score.first);
			
			return;
		} */

		player.autokick(BALL_MAX_SPEED);
	}

	void CShootGoal::draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const {
		ctx->set_line_width(0.025);
		for(auto i : scores) {
			ctx->set_source_rgba(i.second.second.to_degrees() / 15, i.second.second.to_degrees() / 15, i.second.second.to_degrees() / 15, 0.5);
			ctx->arc(i.first.x, i.first.y, 0.02, 0.0, 2*M_PI);
			ctx->fill();
		}
		ctx->set_source_rgba(1.0, 0.0, 0.0, 1.0);
		ctx->arc(best_score.first.x, best_score.first.y, 0.02, 0.0, 2*M_PI);
		ctx->fill();
	}
}

Tactic::Ptr AI::HL::STP::Tactic::cshoot_goal(World world, bool force) {
	Tactic::Ptr p(new CShootGoal(world, force));
	return p;
}

