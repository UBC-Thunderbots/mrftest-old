#include "ai/hl/stp/enemy.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/stp/evaluation/enemy.h"
#include "ai/hl/stp/evaluation/ball.h"

using namespace AI::HL::W;
using AI::HL::STP::Enemy;

namespace Evaluation = AI::HL::STP::Evaluation;

namespace {
	BoolParam use_grab_ball_pos("enemy closest use grab ball position", "STP/enemy", true);

	class Fixed : public Enemy {
		public:
			Fixed(Robot r) : robot(r) {
			}

		private:
			Robot robot;
			Robot evaluate() const {
				return robot;
			}
	};

	class ClosestFriendlyGoal : public Enemy {
		public:
			ClosestFriendlyGoal(World w, unsigned int i) : world(w), index(i) {
			}

		private:
			World world;
			unsigned int index;
			Robot evaluate() const {
				if (world.enemy_team().size() <= index) {
					return Robot();
				}

				std::vector<Robot> enemies = AI::HL::Util::get_robots(world.enemy_team());

				// sort enemies by distance to own goal
				// TODO: cache this
				std::sort(enemies.begin(), enemies.end(), AI::HL::Util::CmpDist<Robot>(world.field().friendly_goal()));

				return enemies[index];
			}
	};

	class ClosestBall : public Enemy {
		public:
			ClosestBall(World w, unsigned int i) : world(w), index(i) {
			}

		private:
			World world;
			unsigned int index;
			Robot evaluate() const {
				if (world.enemy_team().size() <= index) {
					return Robot();
				}

				auto enemies = Evaluation::enemies_by_grab_ball_dist();

				return enemies[index];
			}
	};

	class ClosestRobot : public Enemy {
		public:
			ClosestRobot(World w, unsigned int i) : world(w), index(i) {
			}

		private:
			World world;
			unsigned int index;
			Robot evaluate() const {
				// Remember that the closest robot to robot of index i is that robot itself. 
				if (world.enemy_team().size() > index) {
					robots = AI::HL::Util::get_robots(world.enemy_team());
					std::sort(robots.begin(), robots.end(), AI::HL::Util::CmpDist<Robot>(world.enemy_team().get(i).position()));
					return passees[index];
				} 
				return Robot();
			}
	};

	class ClosestFriendlyPlayer : public Enemy {
		public:
			ClosestFriendlyPlayer(World w, Player player, unsigned int i) : world(w), player(player), index(i) {
			}

		private:
			World world;
			Player player;
			unsigned int index;
			Robot evaluate() const {
				if (world.enemy_team().size() <= index) {
					return Robot();
				}

				std::vector<Robot> enemies = AI::HL::Util::get_robots(world.enemy_team());

				// sort enemies by distance to own goal
				// TODO: cache this
				std::sort(enemies.begin(), enemies.end(), AI::HL::Util::CmpDist<Robot>(player.position()));

				return enemies[index];
			}
	};
	
};

Enemy::Ptr AI::HL::STP::Enemy::closest_friendly_goal(World world, unsigned int i) {
	return std::make_shared<ClosestFriendlyGoal>(world, i);
}

Enemy::Ptr AI::HL::STP::Enemy::closest_ball(World world, unsigned int i) {
	return std::make_shared<ClosestBall>(world, i);
}

Enemy::Ptr AI::HL::STP::Enemy::closest_pass(World world, unsigned int i) {
	return std::make_shared<ClosestPass>(world, i);
}

Enemy::Ptr AI::HL::STP::Enemy::closest_friendly_player(World world, Player player, unsigned int i) {
	return std::make_shared<ClosestFriendlyPlayer>(world, player, i);
}

Enemy::Enemy() = default;

