#include "ai/hl/stp/tactic/cshoot.h"

#include "../action/dribble.h"
#include "ai/hl/stp/action/catch.h"
#include "ai/hl/stp/action/action.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/player.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/param.h"
#include "geom/util.h"
#include "util/dprint.h"
#include "geom/angle.h"

using AI::HL::STP::BALL_MAX_SPEED;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::STP::Action;
namespace Action = AI::HL::STP::Action;
using namespace AI::HL::W;
using namespace Geom;
using AI::HL::STP::Coordinate;
namespace Evaluation = AI::HL::STP::Evaluation;

namespace {
	class CShootGoal final : public Tactic {
		public:
			explicit CShootGoal(World world, bool force) : Tactic(world), kick_attempted(false), force(force),
			timer(0), best_score(std::make_tuple(Point(0, 0), Point(0, 0), 0))
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
			void execute(caller_t& ca) override;
			void player_changed() override;
			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const override;
			Glib::ustring description() const override {
				return u8"cshoot-goal";
			}

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
		if (kick_attempted && players.count(player())) {
			return player();
		}
		return select_baller(world, players, player());
	}

	void CShootGoal::player_changed() {
		initial_position = player() ? player().position() : Point();
		kick_attempted = false;
	}

	void CShootGoal::execute(caller_t& ca) {
		if (timer >= 0 && !player().has_ball()) {
			timer--;
		}
		else if (timer <= 30 && player().has_ball()) {
			timer++;
		}

		best_score = std::make_tuple(Point(0,0), Point(0,0), 0.0);

		if ((world.ball().position() - player().position()).len() > Robot::MAX_RADIUS + 0.02) {
			initial_position = player().position();
		}

		obstacles.clear();
		scores.clear();
		
		//first: find the best location to shoot at in a circle centered at the baller with a radius of 0.5m
		enemy_goal_positive = world.field().enemy_goal_boundary().first.x > 0.0 ? 
			world.field().enemy_goal_boundary().first : world.field().enemy_goal_boundary().second;
		enemy_goal_negative = world.field().enemy_goal_boundary().first.x < 0.0 ?
			world.field().enemy_goal_boundary().first : world.field().enemy_goal_boundary().second;

		for (auto i : world.enemy_team())
			obstacles.push_back(i.position());
		for (auto i : world.friendly_team())
			if ((i.position() - player().position()).len() > 0.01)
				obstacles.push_back(i.position());

		for (double x = initial_position.x - 0.50; x < initial_position.x + 0.50; x += 0.04) {
			for (double y = initial_position.y - 0.50; y < initial_position.y + 0.50; y += 0.04) {
				if ((Point(x, y) - initial_position).len() < 0.45) {
					std::pair<Point, Angle> sweep_score = angle_sweep_circles(
						Point(x, y),
						enemy_goal_positive,
						enemy_goal_negative,
						obstacles,
						Robot::MAX_RADIUS
					);
					double shoot_score = sweep_score.second.to_degrees();

					for (auto p : world.enemy_team()) {
						if((p.position() - Point(x, y)).len() < Robot::MAX_RADIUS * 2 + 0.03)
							shoot_score = 0.0;
					}

					if (dist(Point(x, y), Seg(enemy_goal_negative, enemy_goal_positive)) < 0.4)
						shoot_score *= 0.1;

					shoot_score -= (Point(x, y) - player().position()).len() * 8;
					scores.push_back(std::make_tuple(Point(x, y), sweep_score.first, shoot_score));
				}
			}
		}

		for (auto &i : scores) {
			if (std::get<2>(i) > std::get<2>(best_score))
				best_score = i;
		}

		target = angle_sweep_circles(
			std::get<0>(best_score), 
			enemy_goal_positive,
			enemy_goal_negative,
			obstacles, 
			Robot::MAX_RADIUS
		).first;

		if (!player().has_ball() && timer < 15) {
			Action::catch_ball(ca, world, player(), target);
			return;
		}

		if (((std::get<0>(best_score) - player().position()).len() > 0.05) ||
				(fabs((player().orientation() - (std::get<1>(best_score) - player().position()).orientation()).to_degrees()) > 2.0)) {
			Action::dribble(ca, world, player(), std::get<0>(best_score), std::get<1>(best_score));
			return;
		}

		Action::shoot_target(ca, world, player(), std::get<0>(best_score));
	}

	void CShootGoal::draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const {
		for(auto i : scores) {
			ctx->set_source_rgba(std::get<2>(i) / 15, std::get<2>(i) / 15, std::get<2>(i) / 15, 0.5);
			ctx->arc(std::get<0>(i).x, std::get<0>(i).y, 0.02, 0.0, 2 * M_PI);
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

