#include "ai/hl/stp/evaluate/enemy.h"
#include "ai/hl/util.h"

using namespace AI::HL::W;
using AI::HL::STP::Evaluate::EnemyRole;

namespace {
	class Fixed : public EnemyRole {
		public:
			Robot::Ptr robot;
			Fixed(Robot::Ptr r) : robot(r) {
			}
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
			World& world;
			unsigned int index;
			ClosestFriendlyGoal(World& w, unsigned int i) : world(w), index(i) {
			}
			Robot::Ptr evaluate() const {
				std::vector<Robot::Ptr> enemies = AI::HL::Util::get_robots(world.enemy_team());

				// sort enemies by distance to own goal
				std::sort(enemies.begin(), enemies.end(), AI::HL::Util::CmpDist<Robot::Ptr>(world.field().friendly_goal()));

				if (enemies.size() >= index) {
					return Robot::Ptr();
				}

				return enemies[index];
			}
	};
};

EnemyRole::Ptr AI::HL::STP::Evaluate::EnemyRole::closest_friendly_goal(World& world, unsigned int i) {
	EnemyRole::Ptr p(new ClosestFriendlyGoal(world, i));
	return p;
}

EnemyRole::EnemyRole() {
}

EnemyRole::~EnemyRole() {
}

