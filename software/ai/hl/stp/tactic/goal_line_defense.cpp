#include "ai/hl/stp/tactic/goal_line_defense.h"
#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/action/move.h"
#include "ai/common/robot.h"
#include "ai/hl/util.h"
#include "util/param.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	DoubleParam dist_from_goal_factor(u8"goal line defense dist from goal factor", u8"AI/HL/STP/Action/defend", 1.25, 0.0, 8.0);

	class GoalLineDefense final : public Tactic {
		public:
  	  explicit GoalLineDefense(World world, std::string position) : Tactic(world), position(position){
  		}

		private:
			Player select(const std::set<Player> &players) const override;
			Point getDefendPoint() const;
			Point getDefendPointTangent() const;
			void execute(caller_t& ca) override;
			const std::string position;

			Glib::ustring description() const override {
				std::string play_name = "goal line defense";
				return play_name.append(position);
			}
	};

	# warning improve player select functions in general. Look at hungarian again?
	Player GoalLineDefense::select(const std::set<Player> &players) const{
  	return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(getDefendPoint()));
	}

	Point GoalLineDefense::getDefendPoint() const {
		Point ball_goal_traj;
		Point base = position == "bottom" ? world.field().friendly_goal_boundary().first :
			world.field().friendly_goal_boundary().second;
		ball_goal_traj = base - world.ball().position();
		const double goal_crease_radius = world.field().defense_area_radius() + world.field().defense_area_stretch() / 2;
		Point goal_boundary_trag = - ball_goal_traj.norm(dist_from_goal_factor * goal_crease_radius);
		const double friendly_outline_boundry = -world.field().length() / 2;

	//	if(ball_goal_traj.len() < goal_crease_radius || ball_goal_traj.x < friendly_outline_boundry) {
	//		Point ball_goal_center_traj = world.field().friendly_goal() - world.ball().position();
	//		return Point(ball_goal_center_traj.x - goal_crease_radius, ball_goal_center_traj.y + world.field().defense_area_stretch() / 2);
	//	}

		return base + goal_boundary_trag;
	}

	Point GoalLineDefense::getDefendPointTangent() const {
		const double goal_crease_radius = world.field().defense_area_radius() + world.field().defense_area_stretch() / 2;
		const double gap_between_defenders = AI::Common::Robot::MAX_RADIUS * 2;
		Point ball_goal_traj = world.field().friendly_goal() - world.ball().position();
		Point ball_crease_traj = ball_goal_traj.norm() * (ball_goal_traj.len() - goal_crease_radius);

		Point pos_offset = ball_goal_traj.perp().norm(gap_between_defenders);
		return position == "bottom" ? ball_crease_traj - pos_offset : ball_crease_traj + pos_offset;
	}

	void GoalLineDefense::execute(caller_t& ca) {
		while(true) {
			Point defend_point = getDefendPoint();
		  AI::HL::STP::Action::move(ca, world, player(), defend_point, (world.ball().position() - player().position()).orientation());
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
