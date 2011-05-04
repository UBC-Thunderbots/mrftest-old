#include "ai/hl/stp/enemy.h"
#include "ai/hl/util.h"
#include "ai/hl/stp/evaluation/offense.h"
#include "ai/hl/stp/evaluation/enemy.h"

using namespace AI::HL::W;
using AI::HL::STP::Enemy;

namespace Evaluation = AI::HL::STP::Evaluation;
using AI::HL::STP::Evaluation::EnemyThreat;

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
			ClosestFriendlyGoal(const World &w, unsigned int i) : world(w), index(i) {
			}

		private:
			const World &world;
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
			ClosestBall(const World &w, unsigned int i) : world(w), index(i) {
			}

		private:
			const World &world;
			unsigned int index;
			Robot::Ptr evaluate() const {
			
				if (world.enemy_team().size() <= index) {
					return Robot::Ptr();
				}

				std::vector<Robot::Ptr> enemies = AI::HL::Util::get_robots(world.enemy_team());

				// sort enemies by distance to ball
				// TODO: cache this
				std::sort(enemies.begin(), enemies.end(), AI::HL::Util::CmpDist<Robot::Ptr>(world.ball().position()));

				return enemies[index];
			}
	};
	
	class ClosestPass : public Enemy {
		public:
			ClosestPass(const World &w, const Robot::Ptr r, unsigned int i) : world(w), robot(r), index(i) {
			}

		private:
			const World &world;
			const Robot::Ptr robot;
			unsigned int index;
			Robot::Ptr evaluate() const {
			
				std::vector<Robot::Ptr> enemies = Evaluation::eval_enemy(world, robot).passees;
				if (enemies.size() <= index) {
					if (world.enemy_team().size() > index){
						enemies = AI::HL::Util::get_robots(world.enemy_team());
						std::sort(enemies.begin(), enemies.end(), AI::HL::Util::CmpDist<Robot::Ptr>(world.ball().position()));
						return enemies[index];
					} else {
						return Robot::Ptr();
					}	
				}
				
				return enemies[index];
			}
	};
};

Enemy::Ptr AI::HL::STP::Enemy::closest_friendly_goal(const World &world, unsigned int i) {
	Enemy::Ptr p(new ClosestFriendlyGoal(world, i));
	return p;
}

/*
Enemy::Ptr AI::HL::STP::Enemy::robot(Robot::Ptr r) {
    Enemy::Ptr p(new Fixed(r));
    return p;
}
*/ 

Enemy::Ptr AI::HL::STP::Enemy::closest_ball(const World &world, unsigned int i) {
	Enemy::Ptr p(new ClosestBall(world, i));
	return p;
}

Enemy::Ptr AI::HL::STP::Enemy::closest_pass(const World &world, const Robot::Ptr r, unsigned int i) {
	Enemy::Ptr p(new ClosestPass(world, r, i));
	return p;
}

Enemy::Enemy() {
}

