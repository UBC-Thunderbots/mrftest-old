#include "ai/hl/stp/enemy.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/stp/evaluation/enemy.h"
#include "ai/hl/stp/evaluation/ball.h"
#include <algorithm>

using namespace AI::HL::W;
using AI::HL::STP::Enemy;

namespace Evaluation = AI::HL::STP::Evaluation;

namespace {
	BoolParam use_grab_ball_pos(u8"enemy closest use grab ball position", u8"STP/enemy", true);

	class Fixed final : public Enemy {
		public:
			explicit Fixed(Robot r) : robot(r) {
			}

		private:
			Robot robot;
			Robot evaluate() const override {
				return robot;
			}
	};

	class ClosestFriendlyGoal final : public Enemy {
		public:
			explicit ClosestFriendlyGoal(World w, unsigned int i) : world(w), index(i) {
			}

		private:
			World world;
			unsigned int index;
			Robot evaluate() const override {
				if (world.enemy_team().size() <= index) {
					return Robot();
				}

				std::vector<Robot> enemies = AI::HL::Util::get_robots(world.enemy_team());

				// sort enemies by distance to own goal
				// TODO: cache this
				std::nth_element(enemies.begin(), enemies.begin() + index, enemies.end(), AI::HL::Util::CmpDist<Robot>(world.field().friendly_goal()));

				return enemies[index];
			}
	};

	class ClosestBall final : public Enemy {
		public:
			explicit ClosestBall(World w, unsigned int i) : world(w), index(i) {
			}

		private:
			World world;
			unsigned int index;
			Robot evaluate() const override {
				if (world.enemy_team().size() <= index) {
					return Robot();
				}

				auto enemies = Evaluation::enemies_by_grab_ball_dist();

				return enemies[index];
			}
	};

	class ClosestRobot final : public Enemy {
		public:
			explicit ClosestRobot(World w, unsigned int robot, unsigned int i) : world(w), robot(robot), index(i) {
			}

		private:
			World world;
			unsigned int robot;
			unsigned int index;
			Robot evaluate() const override {
				// Remember that the closest robot to robot of index i is that robot itself. 
				if (world.enemy_team().size() > index) {
					auto robots = AI::HL::Util::get_robots(world.enemy_team());
					std::nth_element(robots.begin(), robots.begin() + index, robots.end(), AI::HL::Util::CmpDist<Robot>(world.enemy_team()[robot].position()));
					return robots[index];
				} 
				return Robot();
			}
	};

	class ClosestFriendlyPlayer final : public Enemy {
		public:
			explicit ClosestFriendlyPlayer(World w, Player player, unsigned int i) : world(w), player(player), index(i) {
			}

		private:
			World world;
			Player player;
			unsigned int index;
			Robot evaluate() const override {
				if (world.enemy_team().size() <= index) {
					return Robot();
				}

				std::vector<Robot> enemies = AI::HL::Util::get_robots(world.enemy_team());

				// sort enemies by distance to own goal
				// TODO: cache this
				std::nth_element(enemies.begin(), enemies.begin() + index, enemies.end(), AI::HL::Util::CmpDist<Robot>(player.position()));

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

Enemy::Ptr AI::HL::STP::Enemy::closest_robot(World world, unsigned int robot, unsigned int i) {
	return std::make_shared<ClosestRobot>(world, robot, i);
}

Enemy::Ptr AI::HL::STP::Enemy::closest_friendly_player(World world, Player player, unsigned int i) {
	return std::make_shared<ClosestFriendlyPlayer>(world, player, i);
}

Enemy::Enemy() = default;

