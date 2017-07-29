#include "ai/hl/stp/tactic/goal_line_defense.h"
#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/action/move.h"
#include "ai/common/robot.h"
#include "ai/hl/util.h"
#include "util/param.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	DoubleParam goal_line_defense_dist_from_goal_factor(u8"goal line defense dist from goal factor", u8"AI/HL/STP/Tactic/goal_line_defense", 1.15, 0.0, 10);
	DoubleParam goal_line_defense_gap_between_defenders(u8"goal line defense gap between defenders", u8"AI/HL/STP/Tactic/goal_line_defense", AI::Common::Robot::MAX_RADIUS, 0.0, 5);

	class GoalLineDefense final : public Tactic {
		public:
  	  explicit GoalLineDefense(World world, std::string position) : Tactic(world), position(position),
				dist_from_goal_factor(goal_line_defense_dist_from_goal_factor),
				gap_between_defenders(goal_line_defense_gap_between_defenders){
  		}

		private:
			Player select(const std::set<Player> &players) const override;
			Point getDefendPoint(std::string role) const;
			Point getDefendPoint() const;
			void execute(caller_t& ca) override;
			const std::string position;
			const double dist_from_goal_factor;
			const double gap_between_defenders;

			Glib::ustring description() const override {
				std::string play_name = "goal line defense ";
				return play_name.append(position);
			}
	};

	Player GoalLineDefense::select(const std::set<Player> &players) const {
  	return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(getDefendPoint()));
	}

	Point GoalLineDefense::getDefendPoint(std::string role) const {
		Point ball_goalside_traj;
		Point goalside_traj;
		const double goal_crease_radius = world.field().defense_area_radius() + world.field().defense_area_stretch() / 2;
		Point offset;

		if (role == "bottom") {
			goalside_traj = world.field().friendly_goal_boundary().first;
			ball_goalside_traj = goalside_traj - world.ball().position();
		} else {
			goalside_traj = world.field().friendly_goal_boundary().second;
			ball_goalside_traj = goalside_traj - world.ball().position();
		}

		Point goalside_defpoint_traj = -ball_goalside_traj.norm(dist_from_goal_factor * goal_crease_radius);
		Point ball_defpoint_traj = goalside_traj + goalside_defpoint_traj;

		return ball_defpoint_traj;
	}

	Point GoalLineDefense::getDefendPoint() const {
		Point defend_point_this = getDefendPoint(position);
		const Point defend_point_other = getDefendPoint(position == "top" ? "bottom" : "top");
		const Point this_other_traj = defend_point_this - defend_point_other;
		double offset = this_other_traj.len() > gap_between_defenders ?
			this_other_traj.len() - gap_between_defenders:
			0;
		return defend_point_this = defend_point_this - this_other_traj.norm(offset);
	}

	void GoalLineDefense::execute(caller_t& ca) {
		while(true) {
		  AI::HL::STP::Action::move(ca, world, player(), getDefendPoint(), (world.ball().position() - player().position()).orientation());
			yield(ca);
			player().clear_prims();
		}
	}
}

Tactic::Ptr AI::HL::STP::Tactic::goal_line_defense_top(World world) {
  Tactic::Ptr p(new GoalLineDefense(world, "top"));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::goal_line_defense_bottom(World world) {
  Tactic::Ptr p(new GoalLineDefense(world, "bottom"));
	return p;
}
