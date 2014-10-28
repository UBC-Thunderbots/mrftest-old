#ifndef AI_HL_STP_PREDICATES
#define AI_HL_STP_PREDICATES

#include "ai/hl/stp/region.h"
#include "ai/hl/stp/world.h"
#include "util/cacheable.h"
#include <functional>

namespace {
    extern DoubleParam chip_estimate;
}

namespace AI {
	namespace HL {
		namespace STP {
			/**
			 * A predicate is a function that returns true or false
			 * depending on the world condition.
			 * A play is more READABLE if its conditions are written as functions.
			 */
			namespace Predicates {
				class Goal final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world) override;
				};

				extern Goal goal;

				class Playtype final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<AI::Common::PlayType>> {
					protected:
						bool compute(World world, AI::Common::PlayType playtype) override;
				};

				extern Playtype playtype;

				class OurBall final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world) override;
				};

				extern OurBall our_ball;

				class TheirBall final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world) override;
				};

				extern TheirBall their_ball;

				class NoneBall final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world) override;
				};

				extern NoneBall none_ball;

				class OurTeamSizeAtLeast final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<unsigned int>> {
					protected:
						bool compute(World world, unsigned int n) override;
				};

				extern OurTeamSizeAtLeast our_team_size_at_least;

				class OurTeamSizeExactly final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<unsigned int>> {
					protected:
						bool compute(World world, unsigned int n) override;
				};

				extern OurTeamSizeExactly our_team_size_exactly;

				class TheirTeamSizeAtMost final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<unsigned int>> {
					protected:
						bool compute(World world, unsigned int n) override;
				};

				extern TheirTeamSizeAtMost their_team_size_at_most;

				class TheirTeamSizeAtLeast final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<unsigned int>> {
					protected:
						bool compute(World world, unsigned int n) override;
				};

				extern TheirTeamSizeAtLeast their_team_size_at_least;

				class BallXLessThan final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<double>> {
					protected:
						bool compute(World world, double x) override;
				};

				extern BallXLessThan ball_x_less_than;

				class BallXGreaterThan final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<double>> {
					protected:
						bool compute(World world, double x) override;
				};

				extern BallXGreaterThan ball_x_greater_than;

				class BallOnOurSide final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world) override;
				};

				extern BallOnOurSide ball_on_our_side;

				class BallOnTheirSide final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world) override;
				};

				extern BallOnTheirSide ball_on_their_side;

				class BallInOurCorner final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world) override;
				};

				extern BallInOurCorner ball_in_our_corner;

				class BallInTheirCorner final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world) override;
				};

				extern BallInTheirCorner ball_in_their_corner;

				class BallMidfield final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world) override;
				};

				extern BallMidfield ball_midfield;

				class BallNearFriendlyGoal final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world) override;
				};

				extern BallNearFriendlyGoal ball_near_friendly_goal;

				class BallNearEnemyGoal final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world) override;
				};

				extern BallNearEnemyGoal ball_near_enemy_goal;

				/**
				 * player with the ball can shoot at their goal
				 */
				class BallerCanShoot final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world) override;
				};

				extern BallerCanShoot baller_can_shoot;

				/**
				 * player with the ball can chip
				 */
				class BallerCanChip final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<bool>> {
					protected:
						bool compute(World world, bool towardsEnemy) override;
				};

				extern BallerCanChip baller_can_chip;

				/**
				 * player with the ball can pass
				 */
				class BallerCanPass final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world) override;
				};

				extern BallerCanPass baller_can_pass;

				/**
				 * player with the ball can shoot at a target point
				 */
				class BallerCanPassTarget final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<Point>> {
					protected:
						bool compute(World world, Point target) override;
				};

				extern BallerCanPassTarget baller_can_pass_target;

				/**
				 * player with the ball is under threat (surrounded by enemies)
				 */
				class BallerUnderThreat final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world) override;
				};

				extern BallerUnderThreat baller_under_threat;

				/**
				 * enemy with the ball can shoot at our goal
				 */
				class EnemyBallerCanShoot final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world) override;
				};

				extern EnemyBallerCanShoot enemy_baller_can_shoot;

				/**
				 * enemy with the ball can pass to another enemy
				 */
				class EnemyBallerCanPass final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world) override;
				};

				extern EnemyBallerCanPass enemy_baller_can_pass;

				/**
				 * # of passes from the enemy with ball is > 0 (no clear shot)
				 * and < 3 (irrelevant to be any higher)
				 * to be able to get a clear shoot at our goal
				 */
				class EnemyBallerCanPassShoot final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world) override;
				};

				extern EnemyBallerCanPassShoot enemy_baller_can_pass_shoot;

				/**
				 * borrowed from cm, true if our ball or ball on their side
				 */
				class Offensive final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world) override;
				};

				extern Offensive offensive;

				/**
				 * borrowed from cm, true if their ball or ball on our side
				 */
				class Defensive final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world) override;
				};

				extern Defensive defensive;

				/**
				 * borrowed from cm, true if number of enemies on our side is greater than n
				 */
				class NumOfEnemiesOnOurSideAtLeast final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<unsigned int>> {
					protected:
						bool compute(World world, unsigned int n) override;
				};

				extern NumOfEnemiesOnOurSideAtLeast num_of_enemies_on_our_side_at_least;

				/**
				 * true if ball is inside region
				 */
				class BallInsideRegion final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<Region>> {
					protected:
						bool compute(World world, Region region) override;
				};

				extern BallInsideRegion ball_inside_region;

				/**
				 * true if our_ball and their_ball (so we have to fight for the ball)
				 */
				class FightBall final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world) override;
				};

				extern FightBall fight_ball;

				class CanShootRay final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world) override;
				};

				extern CanShootRay can_shoot_ray;

				/**
				 * true if the ball is inside some robot
				 */
				class BallInsideRobot final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world) override;
				};

				extern BallInsideRobot ball_inside_robot;

				class EnemyBreakDefenseDuo final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world) override;
				};

				extern EnemyBreakDefenseDuo enemy_break_defense_duo;

				class BallTowardsEnemy final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world) override;
				};

				extern BallTowardsEnemy ball_towards_enemy;

				class BallOnEnemyNet final : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world) override;
				};

				extern BallOnEnemyNet ball_on_enemy_net;

				class LooseBall final : public Cacheable<bool,
CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world) override;
				};
				
				extern LooseBall loose_ball;
			}
		}
	}
}

#endif

