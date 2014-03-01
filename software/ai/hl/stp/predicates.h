#ifndef AI_HL_STP_PREDICATES
#define AI_HL_STP_PREDICATES

#include "ai/hl/stp/region.h"
#include "ai/hl/stp/world.h"
#include "util/cacheable.h"
#include <functional>

namespace AI {
	namespace HL {
		namespace STP {
			/**
			 * A predicate is a function that returns true or false
			 * depending on the world condition.
			 * A play is more READABLE if its conditions are written as functions.
			 */
			namespace Predicates {
				class Goal : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world);
				};

				extern Goal goal;

				class Playtype : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<AI::Common::PlayType>> {
					protected:
						bool compute(World world, AI::Common::PlayType playtype);
				};

				extern Playtype playtype;

				class OurBall : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world);
				};

				extern OurBall our_ball;

				class TheirBall : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world);
				};

				extern TheirBall their_ball;

				class NoneBall : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world);
				};

				extern NoneBall none_ball;

				class OurTeamSizeAtLeast : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<unsigned int>> {
					protected:
						bool compute(World world, unsigned int n);
				};

				extern OurTeamSizeAtLeast our_team_size_at_least;

				class OurTeamSizeExactly : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<unsigned int>> {
					protected:
						bool compute(World world, unsigned int n);
				};

				extern OurTeamSizeExactly our_team_size_exactly;

				class TheirTeamSizeAtMost : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<unsigned int>> {
					protected:
						bool compute(World world, unsigned int n);
				};

				extern TheirTeamSizeAtMost their_team_size_at_most;

				class TheirTeamSizeAtLeast : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<unsigned int>> {
					protected:
						bool compute(World world, unsigned int n);
				};

				extern TheirTeamSizeAtLeast their_team_size_at_least;

				class BallXLessThan : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<double>> {
					protected:
						bool compute(World world, double x);
				};

				extern BallXLessThan ball_x_less_than;

				class BallXGreaterThan : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<double>> {
					protected:
						bool compute(World world, double x);
				};

				extern BallXGreaterThan ball_x_greater_than;

				class BallOnOurSide : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world);
				};

				extern BallOnOurSide ball_on_our_side;

				class BallOnTheirSide : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world);
				};

				extern BallOnTheirSide ball_on_their_side;

				class BallInOurCorner : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world);
				};

				extern BallInOurCorner ball_in_our_corner;

				class BallInTheirCorner : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world);
				};

				extern BallInTheirCorner ball_in_their_corner;

				class BallMidfield : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world);
				};

				extern BallMidfield ball_midfield;

				class BallNearFriendlyGoal : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world);
				};

				extern BallNearFriendlyGoal ball_near_friendly_goal;

				class BallNearEnemyGoal : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world);
				};

				extern BallNearEnemyGoal ball_near_enemy_goal;

				/**
				 * player with the ball can shoot at their goal
				 */
				class BallerCanShoot : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world);
				};

				extern BallerCanShoot baller_can_shoot;

				/**
				 * player with the ball can chip
				 */
				class BallerCanChip : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world);
				};

				extern BallerCanChip baller_can_chip;

				/**
				 * player with the ball can pass
				 */
				class BallerCanPass : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world);
				};

				extern BallerCanPass baller_can_pass;

				/**
				 * player with the ball can shoot at a target point
				 */
				class BallerCanPassTarget : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<Point>> {
					protected:
						bool compute(World world, Point target);
				};

				extern BallerCanPassTarget baller_can_pass_target;

				/**
				 * player with the ball is under threat (surrounded by enemies)
				 */
				class BallerUnderThreat : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world);
				};

				extern BallerUnderThreat baller_under_threat;

				/**
				 * enemy with the ball can shoot at our goal
				 */
				class EnemyBallerCanShoot : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world);
				};

				extern EnemyBallerCanShoot enemy_baller_can_shoot;

				/**
				 * enemy with the ball can pass to another enemy
				 */
				class EnemyBallerCanPass : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world);
				};

				extern EnemyBallerCanPass enemy_baller_can_pass;

				/**
				 * # of passes from the enemy with ball is > 0 (no clear shot)
				 * and < 3 (irrelevant to be any higher)
				 * to be able to get a clear shoot at our goal
				 */
				class EnemyBallerCanPassShoot : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world);
				};

				extern EnemyBallerCanPassShoot enemy_baller_can_pass_shoot;

				/**
				 * borrowed from cm, true if our ball or ball on their side
				 */
				class Offensive : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world);
				};

				extern Offensive offensive;

				/**
				 * borrowed from cm, true if their ball or ball on our side
				 */
				class Defensive : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world);
				};

				extern Defensive defensive;

				/**
				 * borrowed from cm, true if number of enemies on our side is greater than n
				 */
				class NumOfEnemiesOnOurSideAtLeast : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<unsigned int>> {
					protected:
						bool compute(World world, unsigned int n);
				};

				extern NumOfEnemiesOnOurSideAtLeast num_of_enemies_on_our_side_at_least;

				/**
				 * true if ball is inside region
				 */
				class BallInsideRegion : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<Region>> {
					protected:
						bool compute(World world, Region region);
				};

				extern BallInsideRegion ball_inside_region;

				/**
				 * true if our_ball and their_ball (so we have to fight for the ball)
				 */
				class FightBall : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world);
				};

				extern FightBall fight_ball;

				class CanShootRay : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world);
				};

				extern CanShootRay can_shoot_ray;

				/**
				 * true if the ball is inside some robot
				 */
				class BallInsideRobot : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world);
				};

				extern BallInsideRobot ball_inside_robot;

				class EnemyBreakDefenseDuo : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world);
				};

				extern EnemyBreakDefenseDuo enemy_break_defense_duo;

				class BallTowardsEnemy : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world);
				};

				extern BallTowardsEnemy ball_towards_enemy;

				class BallOnEnemyNet : public Cacheable<bool, CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world);
				};

				extern BallOnEnemyNet ball_on_enemy_net;

				class LooseBall : public Cacheable<bool,
CacheableNonKeyArgs<World>, CacheableKeyArgs<>> {
					protected:
						bool compute(World world);
				};
				
				extern LooseBall loose_ball;
			}
		}
	}
}

#endif

