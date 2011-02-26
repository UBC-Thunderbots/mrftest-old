#include "ai/hl/stp/tactic/penalty_shoot.h"
#include "ai/hl/stp/action/actions.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;

namespace {
	class PenaltyShoot : public Tactic {
		public:
			PenaltyShoot(const World &world) : Tactic(world, true) {
			}

		private:
			bool done() const;
			Player::Ptr select(const std::set<Player::Ptr> &players) const;
			void execute();
	};

	bool PenaltyShoot::done() const {
		return false;
	}

	Player::Ptr PenaltyShoot::select(const std::set<Player::Ptr> &players) const {
		for (auto it = players.begin(); it != players.end(); ++it) {
			if ((*it)->has_ball()) {
				return *it;
			}
		}
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player::Ptr>(world.ball().position()));
	}

	void PenaltyShoot::execute() {
		// Find the Enemy Goalie by dist to enemy goal
		std::vector<Robot::Ptr> enemies = AI::HL::Util::get_robots(world.enemy_team());
		if (enemies.size() > 1) {
			std::sort(enemies.begin() + 1, enemies.end(), AI::HL::Util::CmpDist<Robot::Ptr>(world.field().enemy_goal()));
		}
		Robot::Ptr enemy_goalie = enemies[0];
		
		Point goal_pos[2] = {
			Point(world.field().length() / 2, world.field().goal_width() - AI::HL::Util::POS_CLOSE),
			Point(world.field().length() / 2, -world.field().goal_width() - AI::HL::Util::POS_CLOSE)
		};
		
		double thres_dist = 0.5*Robot::MAX_RADIUS;
		
		Point thres_pos[2] = {
			Point(world.field().length() / 2, world.field().goal_width() / 3),
			Point(world.field().length() / 2, -world.field().goal_width() / 3)
		};

		if ((enemy_goalie->position()-thres_pos[0]).len() > thres_dist || (enemy_goalie->position()-thres_pos[1]).len() > thres_dist) {		
			if ((enemy_goalie->position()-goal_pos[0]).len() > (enemy_goalie->position()-goal_pos[1]).len()) {
				AI::HL::STP::Actions::shoot(world, player, AI::Flags::FLAG_CLIP_PLAY_AREA, goal_pos[0], 10.0);
			} else {
				AI::HL::STP::Actions::shoot(world, player, AI::Flags::FLAG_CLIP_PLAY_AREA, goal_pos[1], 10.0);
			}
		} else {
			AI::HL::STP::Actions::shoot(world, player, AI::Flags::FLAG_CLIP_PLAY_AREA, goal_pos[0], 10.0);
		}
				
	}
}

Tactic::Ptr AI::HL::STP::Tactic::penalty_shoot(const World &world) {
	const Tactic::Ptr p(new PenaltyShoot(world));
	return p;
}

