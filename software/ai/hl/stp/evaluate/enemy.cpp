#include "ai/hl/stp/evaluate/enemy.h"
#include "ai/hl/util.h"

using namespace AI::HL::W;
using AI::HL::STP::Evaluate::EnemyRole;

namespace {
	class Fixed : public EnemyRole {
		public:
			Fixed(Robot::Ptr r) : robot(r) {
			}
		private:
			Robot::Ptr robot;
			Robot::Ptr evaluate() const {
				return robot;
			}
	};
};

EnemyRole::Ptr AI::HL::STP::Evaluate::EnemyRole::fixed(Robot::Ptr robot) {
	EnemyRole::Ptr p(new Fixed(robot));
	return p;
}

namespace {
	class ClosestFriendlyGoal : public EnemyRole {
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
};

EnemyRole::Ptr AI::HL::STP::Evaluate::EnemyRole::closest_friendly_goal(World& world, unsigned int i) {
	EnemyRole::Ptr p(new ClosestFriendlyGoal(world, i));
	return p;
}

namespace {
	class ClosestBall : public EnemyRole {
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

EnemyRole::Ptr AI::HL::STP::Evaluate::EnemyRole::closest_ball(World& world, unsigned int i) {
	EnemyRole::Ptr p(new ClosestBall(world, i));
	return p;
}

EnemyRole::EnemyRole() {
}

EnemyRole::~EnemyRole() {
}

