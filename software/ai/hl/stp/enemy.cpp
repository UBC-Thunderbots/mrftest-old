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

				auto enemies = Evaluation::enemies_by_grab_ball_dist();

				return enemies[index];
			}
	};

	class ClosestPass : public Enemy {
		public:
			ClosestPass(const World &w, unsigned int i) : world(w), index(i) {
			}

		private:
			const World &world;
			unsigned int index;
			Robot::Ptr evaluate() const {
				return Robot::Ptr();
				// TODO: redo this
				/*
				   std::vector<Robot::Ptr> passees = Evaluation::get_passees(world, robot);

				   if (passees.size() <= index) {
				   if (world.enemy_team().size() > index) {
				   passees = AI::HL::Util::get_robots(world.enemy_team());
				   std::sort(passees.begin(), passees.end(), AI::HL::Util::CmpDist<Robot::Ptr>(world.ball().position()));
				   return passees[index];
				   } else {
				   return Robot::Ptr();
				   }
				   }

				   return passees[index];
				 */
			}
	};

	class ClosestFriendlyPlayer : public Enemy {
		public:
			ClosestFriendlyPlayer(const World &w, Player::Ptr player, unsigned int i) : world(w), player(player), index(i) {
			}

		private:
			const World &world;
			Player::Ptr player;
			unsigned int index;
			Robot::Ptr evaluate() const {
				if (world.enemy_team().size() <= index) {
					return Robot::Ptr();
				}

				std::vector<Robot::Ptr> enemies = AI::HL::Util::get_robots(world.enemy_team());

				// sort enemies by distance to own goal
				// TODO: cache this
				std::sort(enemies.begin(), enemies.end(), AI::HL::Util::CmpDist<Robot::Ptr>(player->position()));

				return enemies[index];
			}
	};
	
};

Enemy::Ptr AI::HL::STP::Enemy::closest_friendly_goal(const World &world, unsigned int i) {
	return std::make_shared<ClosestFriendlyGoal>(world, i);
}

Enemy::Ptr AI::HL::STP::Enemy::closest_ball(const World &world, unsigned int i) {
	return std::make_shared<ClosestBall>(world, i);
}

Enemy::Ptr AI::HL::STP::Enemy::closest_pass(const World &world, unsigned int i) {
	return std::make_shared<ClosestPass>(world, i);
}

Enemy::Ptr AI::HL::STP::Enemy::closest_friendly_player(const World &world, Player::Ptr player, unsigned int i) {
	return std::make_shared<ClosestFriendlyPlayer>(world, player, i);
}

Enemy::Enemy() = default;

