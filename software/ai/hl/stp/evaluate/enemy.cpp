#include "ai/hl/stp/evaluate/enemy.h"
#include "ai/hl/util.h"

using namespace AI::HL::W;
using AI::HL::STP::Evaluation::Enemy;

namespace {
	class Fixed : public Enemy {
		public:
			Fixed(Robot::Ptr r) : robot(r) {
			}
		private:
			Robot::Ptr robot;
			Robot::Ptr evaluate() const {
				return robot;
			}
	};

	class ClosestFriendlyGoal : public Enemy {
		public:
			ClosestFriendlyGoal(World& w, unsigned int i) : world(w), index(i) {
			}
		private:
			World& world;
			unsigned int index;
			Robot::Ptr evaluate() const {
				if (world.enemy_team().size() <= index) {
					return Robot::Ptr();
				}

				std::vector<Robot::Ptr> enemies = AI::HL::Util::get_robots(world.enemy_team());

				// sort enemies by distance to own goal
				// TODO: cache this
				std::sort(enemies.begin(), enemies.end(), AI::HL::Util::CmpDist<Robot::Ptr>(world.field().friendly_goal()));

				return enemies[index];
			}
	};

	class ClosestBall : public Enemy {
		public:
			ClosestBall(World& w, unsigned int i) : world(w), index(i) {
			}
		private:
			World& world;
			unsigned int index;
			Robot::Ptr evaluate() const {
				if (world.enemy_team().size() <= index) {
					return Robot::Ptr();
				}

				std::vector<Robot::Ptr> enemies = AI::HL::Util::get_robots(world.enemy_team());

				// sort enemies by distance to own goal
				// TODO: cache this
				std::sort(enemies.begin(), enemies.end(), AI::HL::Util::CmpDist<Robot::Ptr>(world.ball().position()));

				return enemies[index];
			}
	};
};

Enemy::Ptr AI::HL::STP::Evaluation::Enemy::closest_friendly_goal(World& world, unsigned int i) {
	Enemy::Ptr p(new ClosestFriendlyGoal(world, i));
	return p;
}

Enemy::Ptr AI::HL::STP::Evaluation::Enemy::robot(Robot::Ptr r) {
	Enemy::Ptr p(new Fixed(r));
	return p;
}

Enemy::Ptr AI::HL::STP::Evaluation::Enemy::closest_ball(World& world, unsigned int i) {
	Enemy::Ptr p(new ClosestBall(world, i));
	return p;
}

Enemy::Enemy() {
}

Enemy::~Enemy() {
}

