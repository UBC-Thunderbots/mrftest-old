#include "ai/hl/stp/tactic/penalty_shoot.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	class PenaltyShoot final : public Tactic
	{
		public:
			explicit PenaltyShoot(World world)
				: Tactic(world, true), shoot_up(true), has_shot(false)
			{
			}

		private:
			bool shoot_up;
			bool has_shot;
			bool done() const override;
			Player select(const std::set<Player> &players) const override;
			void execute() override;
			Glib::ustring description() const override
			{
				return u8"penalty-shoot";
			}
	};

	bool PenaltyShoot::done() const
	{
		return has_shot;
	}

	Player PenaltyShoot::select(const std::set<Player> &players) const
	{
		return *std::min_element(players.begin(), players.end(),
				AI::HL::Util::CmpDist<Player>(world.field().enemy_goal()));
	}

	void PenaltyShoot::execute()
	{
		const Field &f = world.field();
		Point enemy_goal = f.enemy_goal();
		Point enemy_goal_post1 = f.enemy_goal_boundary().first;
		Point enemy_goal_post2 = f.enemy_goal_boundary().second;

		// shoot center of goal if there is no enemy
		// otherwise, find a side to shoot
		if (world.enemy_team().size() > 0)
		{
		    // since all other robots not participating in penalty shoot must be far away from the goal post
		    // hence the enemy goalie is the robot closest to enemy goal post
		    std::vector<Robot> enemies = AI::HL::Util::get_robots(world.enemy_team());

		    Robot enemy_goalie = *std::min_element(enemies.begin(), enemies.end(), AI::HL::Util::CmpDist<Robot>(world.field().enemy_goal()));

		    // a hysteresis
		    double goal_post_diff = (f.enemy_goal_boundary().first - f.enemy_goal_boundary().second).len();
		    const double target_y = goal_post_diff * 3 / 4;

		    if (shoot_up && enemy_goalie.position().y + Robot::MAX_RADIUS > target_y) {
		        shoot_up = false;
		    } else if (!shoot_up && enemy_goalie.position().y - Robot::MAX_RADIUS < -target_y) {
		        shoot_up = true;
		    }

		    if (shoot_up) {
		    	enemy_goal = Point(f.length() / 2, target_y);
		    } else {
		    	enemy_goal = Point(f.length() / 2, -target_y);
		    }
		}
		else
		{
			has_shot = AI::HL::STP::Action::shoot_target(world, player, enemy_goal);
		}

		 //has_shot = AI::HL::STP::Action::shoot_goal(world, player, false);

		// unset any flags
		player.flags(0);
	}
}

Tactic::Ptr AI::HL::STP::Tactic::penalty_shoot(World world)
{
	Tactic::Ptr p(new PenaltyShoot(world));
	return p;
}

