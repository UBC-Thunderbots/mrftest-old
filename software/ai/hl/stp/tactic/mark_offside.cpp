#include "ai/hl/stp/tactic/util.h"
#include "ai/hl/stp/evaluation/enemy.h"
#include "ai/hl/stp/evaluation/team.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/action/block.h"
#include "ai/hl/stp/stp.h"
#include "ai/hl/stp/tactic/mark_offside.h"
#include "ai/hl/stp/action/move.h"
#include <vector>
#include <iostream>

using namespace std;
using namespace AI::HL::STP::Tactic;
using namespace AI::HL::W;
using namespace AI::HL::Util;
using AI::HL::STP::Coordinate;
namespace Action = AI::HL::STP::Action;
namespace Evaluation = AI::HL::STP::Evaluation;

namespace {
	class MarkOffside final : public Tactic {
		public:
			explicit MarkOffside(World world, unsigned i) : Tactic(world), index(i) {
			}

		private:
			Player select(const std::set<Player> &players) const override;
			void execute() override;
			Coordinate dest;
			std::vector<AI::HL::W::Robot> player_to_mark(std::vector<AI::HL::W::Robot> enemies) const;
			Player nearest_friendly(Point target) const;
			unsigned index;
			Glib::ustring description() const override {
				return u8"MarkOffside";
			}
	};

	Player MarkOffside::select(const std::set<Player> &players) const {
		return *std::min_element(players.begin(), players.end(), AI::HL::Util::CmpDist<Player>(dest.position()));
	}

	std::vector<AI::HL::W::Robot> MarkOffside::player_to_mark(std::vector<AI::HL::W::Robot> enemies) const {
#warning This is a quick implementation during robocup 2015 to find more than one mark offender. This should be changed later to not take n^2 time

		// filter out enemies that:
		// 1. are far away from our goal
		// 2. can't shoot to goal
		enemies.erase(std::remove_if(enemies.begin(), enemies.end(), [this](AI::HL::W::Robot robot) -> bool {
			return (robot.position().x > 1.0) || (!Evaluation::enemy_can_shoot_goal(world, robot));
		}), enemies.end());

		std::vector<AI::HL::W::Robot> open_robots;

		 AI::HL::W::Robot most_open_robot = Evaluation::calc_enemy_baller(world);
		for (std::size_t j = 0; j < enemies.size(); j++)
		{
			// find the enemy robot with the biggest open space
			int most_open_index = -1;
			double dist = 0;
			for (std::size_t i = 0; i < enemies.size(); i++) {
				double d = (enemies[i].position() - nearest_friendly(enemies[i].position()).position()).len();
				if (most_open_index < 0 || d > dist) {
					most_open_index = static_cast<int>(i);
					dist = d;
				}
			}

			// every time we find an open robot, we remove it from the list and redo again with a smaller list.
			if (most_open_index != -1) {
				open_robots.push_back(enemies[static_cast<std::size_t>(most_open_index)]);
				enemies.erase(enemies.begin() + most_open_index);
			}
		}

		return open_robots;
	}

	//return nearest friendly from the pool of non-marker players
	Player MarkOffside::nearest_friendly(Point target) const{
		std::vector<Player> team_pool;
		for (Player i : world.friendly_team()) {
			// filter out the player
			if (i == player) {
				team_pool.push_back(i);
			}
		}
		Player closest;
		double dist = 0;
		for (std::size_t i = 0; i < team_pool.size(); i++) {
			double d = (target - team_pool[i].position()).len();
			if (!closest || d < dist) {
				closest = team_pool[i];
				dist = d;
			}
		}
		return closest;
	}

	void MarkOffside::execute() {
		if (world.enemy_team().size()) {
			std::vector<Robot> players_to_mark = AI::HL::Util::get_robots(world.enemy_team());
			if (index < players_to_mark.size())
			{
				Robot player_to_mark = AI::HL::Util::get_robots(world.enemy_team())[index];
				Action::block_ball(world, player, player_to_mark);
			}
			else {
				Action::move(player, (player.position() - world.ball().position()).orientation(), Point(world.ball().position().x, -world.ball().position().y));
			}
		} else {
			Action::move(player, (player.position() - world.ball().position()).orientation(), Point(world.ball().position().x, -world.ball().position().y));
		}
	}
}


Tactic::Ptr AI::HL::STP::Tactic::mark_offside(World world) {
	Tactic::Ptr p(new MarkOffside(world, 0));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::mark_offside1(World world) {
	Tactic::Ptr p(new MarkOffside(world, 1));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::mark_offside2(World world) {
	Tactic::Ptr p(new MarkOffside(world, 2));
	return p;
}

Tactic::Ptr AI::HL::STP::Tactic::mark_offside3(World world) {
	Tactic::Ptr p(new MarkOffside(world, 3));
	return p;
}

