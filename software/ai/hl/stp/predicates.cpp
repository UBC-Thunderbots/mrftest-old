#include "ai/hl/stp/predicates.h"
#include "ai/hl/util.h"
#include "geom/angle.h"
#include "ai/hl/stp/evaluation/enemy.h"
#include "ai/hl/stp/evaluation/pass.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/ball_threat.h"
#include "ai/hl/stp/evaluation/defense.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/param.h"

#include <set>

using namespace AI::HL::STP;

namespace {
	DoubleParam near_thresh("enemy avoidance distance (robot radius)", "STP/predicates", 3.0, 1.0, 10.0);
	DoubleParam fight_thresh("dist thresh to start fight ball with enemy (robot radius)", "STP/predicates", 2.0, 0.1, 4.0);

	BoolParam new_fight("new fight", "STP/predicates", true);
}

bool Predicates::Goal::compute(const World &) {
	return false;
}

Predicates::Goal Predicates::goal;

bool Predicates::Playtype::compute(const World &world, AI::Common::PlayType playtype) {
	return world.playtype() == playtype;
}

Predicates::Playtype Predicates::playtype;

bool Predicates::OurBall::compute(const World &world) {
	const FriendlyTeam &friendly = world.friendly_team();
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		if (Evaluation::possess_ball(world, friendly.get(i))) {
			return true;
		}
	}
	return false;
}

Predicates::OurBall Predicates::our_ball;

bool Predicates::TheirBall::compute(const World &world) {
	const EnemyTeam &enemy = world.enemy_team();
	for (std::size_t i = 0; i < enemy.size(); ++i) {
		if (Evaluation::possess_ball(world, enemy.get(i))) {
			return true;
		}
	}
	return false;
}

Predicates::TheirBall Predicates::their_ball;

bool Predicates::NoneBall::compute(const World &world) {
	return !our_ball(world) && !their_ball(world);
}

Predicates::NoneBall Predicates::none_ball;

bool Predicates::OurTeamSizeAtLeast::compute(const World &world, const unsigned int n) {
	return world.friendly_team().size() >= n;
}

Predicates::OurTeamSizeAtLeast Predicates::our_team_size_at_least;

bool Predicates::OurTeamSizeExactly::compute(const World &world, const unsigned int n) {
	return world.friendly_team().size() == n;
}

Predicates::OurTeamSizeExactly Predicates::our_team_size_exactly;

bool Predicates::TheirTeamSizeAtLeast::compute(const World &world, const unsigned int n) {
	return world.enemy_team().size() >= n;
}

Predicates::TheirTeamSizeAtLeast Predicates::their_team_size_at_least;

bool Predicates::TheirTeamSizeAtMost::compute(const World &world, const unsigned int n) {
	return world.enemy_team().size() <= n;
}

Predicates::TheirTeamSizeAtMost Predicates::their_team_size_at_most;

bool Predicates::BallXLessThan::compute(const World &world, const double x) {
	return world.ball().position().x < x;
}

Predicates::BallXLessThan Predicates::ball_x_less_than;

bool Predicates::BallXGreaterThan::compute(const World &world, const double x) {
	return world.ball().position().x > x;
}

Predicates::BallXGreaterThan Predicates::ball_x_greater_than;

bool Predicates::BallOnOurSide::compute(const World &world) {
	return world.ball().position().x <= 0;
}

Predicates::BallOnOurSide Predicates::ball_on_our_side;

bool Predicates::BallOnTheirSide::compute(const World &world) {
	return world.ball().position().x > 0;
}

Predicates::BallOnTheirSide Predicates::ball_on_their_side;

bool Predicates::BallInOurCorner::compute(const World &world) {
	return world.ball().position().x <= -world.field().length() / 4 && std::fabs(world.ball().position().y) > world.field().goal_width() / 2 + world.field().defense_area_radius();
}

Predicates::BallInOurCorner Predicates::ball_in_our_corner;

bool Predicates::BallInTheirCorner::compute(const World &world) {
	return world.ball().position().x >= world.field().length() / 4 && std::fabs(world.ball().position().y) > world.field().goal_width() / 2 + world.field().defense_area_radius();
}

Predicates::BallInTheirCorner Predicates::ball_in_their_corner;

bool Predicates::BallMidfield::compute(const World &world) {
	return std::fabs(world.ball().position().x) < world.field().length() / 4;
}

Predicates::BallMidfield Predicates::ball_midfield;

bool Predicates::BallNearFriendlyGoal::compute(const World &world) {
	return ball_on_our_side(world) && !ball_in_our_corner(world) && !ball_midfield(world);
}

Predicates::BallNearFriendlyGoal Predicates::ball_near_friendly_goal;

bool Predicates::BallNearEnemyGoal::compute(const World &world) {
	return ball_on_their_side(world) && !ball_in_their_corner(world) && !ball_midfield(world);
}

Predicates::BallNearEnemyGoal Predicates::ball_near_enemy_goal;

bool Predicates::BallerCanShoot::compute(const World &world) {
	const Player::CPtr baller = Evaluation::calc_friendly_baller();
	if (!baller || !Evaluation::possess_ball(world, baller)) {
		return false;
	}
	return Evaluation::evaluate_shoot(world, baller).angle >= min_shoot_region;
}

Predicates::BallerCanShoot Predicates::baller_can_shoot;

bool Predicates::BallerCanPassTarget::compute(const World &world, const Point target) {
	const Player::CPtr baller = Evaluation::calc_friendly_baller();
	if (!baller || !Evaluation::possess_ball(world, baller)) {
		return false;
	}
#warning something is wierd
	return Evaluation::can_pass(world, baller->position(), target);
}

Predicates::BallerCanPassTarget Predicates::baller_can_pass_target;

bool Predicates::BallerCanPass::compute(const World &world) {
	const Player::CPtr baller = Evaluation::calc_friendly_baller();
	if (!baller || !Evaluation::possess_ball(world, baller)) {
		return false;
	}
	return Evaluation::select_passee(world);
}

Predicates::BallerCanPass Predicates::baller_can_pass;

bool Predicates::BallerUnderThreat::compute(const World &world) {
	const Player::CPtr baller = Evaluation::calc_friendly_baller();
	if (!baller || !Evaluation::possess_ball(world, baller)) {
		return false;
	}
	int enemy_cnt = 0;
	const EnemyTeam &enemies = world.enemy_team();
	for (std::size_t i = 0; i < enemies.size(); ++i) {
		if ((baller->position() - enemies.get(i)->position()).len() <= near_thresh * AI::HL::W::Robot::MAX_RADIUS) {
			enemy_cnt++;
		}
	}

	return enemy_cnt >= 2;
}

Predicates::BallerUnderThreat Predicates::baller_under_threat;

bool Predicates::EnemyBallerCanShoot::compute(const World &world) {
	const Robot::Ptr baller = Evaluation::calc_enemy_baller(world);
	if (!baller || !Evaluation::possess_ball(world, baller)) {
		return false;
	}
	return Evaluation::enemy_can_shoot_goal(world, baller);
}

Predicates::EnemyBallerCanShoot Predicates::enemy_baller_can_shoot;

bool Predicates::EnemyBallerCanPass::compute(const World &world) {
	const Robot::Ptr baller = Evaluation::calc_enemy_baller(world);
	if (!baller || !Evaluation::possess_ball(world, baller)) {
		return false;
	}
	return Evaluation::calc_enemy_pass(world, baller) > 0;
}

Predicates::EnemyBallerCanPass Predicates::enemy_baller_can_pass;

bool Predicates::EnemyBallerCanPassShoot::compute(const World &world) {
	const Robot::Ptr baller = Evaluation::calc_enemy_baller(world);
	if (!baller || !Evaluation::possess_ball(world, baller)) {
		return false;
	}
	return Evaluation::calc_enemy_pass(world, baller) > 0 && Evaluation::calc_enemy_pass(world, baller) < 3;
}

Predicates::EnemyBallerCanPassShoot Predicates::enemy_baller_can_pass_shoot;

bool Predicates::Offensive::compute(const World &world) {
	return our_ball(world) || ball_on_their_side(world);
}

Predicates::Offensive Predicates::offensive;

bool Predicates::Defensive::compute(const World &world) {
	return (their_ball(world) || ball_on_our_side(world)) && !offensive(world);
}

Predicates::Defensive Predicates::defensive;

bool Predicates::NumOfEnemiesOnOurSideAtLeast::compute(const World &world, const unsigned int n) {
	unsigned int cnt = 0;
	const EnemyTeam &enemies = world.enemy_team();
	for (std::size_t i = 0; i < enemies.size(); ++i) {
		if (enemies.get(i)->position().x < 0) {
			cnt++;
		}
	}
	if (cnt >= n) {
		return true;
	}
	return false;
}

Predicates::NumOfEnemiesOnOurSideAtLeast Predicates::num_of_enemies_on_our_side_at_least;

bool Predicates::BallInsideRegion::compute(const World &world, Region region) {
	return region.inside(world.ball().position());
}

Predicates::BallInsideRegion Predicates::ball_inside_region;

bool Predicates::FightBall::compute(const World &world) {
	if (!new_fight) {
		return our_ball(world) && their_ball(world);
	} else {
		const Player::CPtr friendly_baller = Evaluation::calc_friendly_baller();
		const Robot::Ptr enemy_baller = Evaluation::calc_enemy_baller(world);

		return (friendly_baller && enemy_baller)
		       && (friendly_baller->position() - world.ball().position()).len() < fight_thresh * Robot::MAX_RADIUS
		       && (enemy_baller->position() - world.ball().position()).len() < fight_thresh * Robot::MAX_RADIUS;
	}
}

Predicates::FightBall Predicates::fight_ball;

bool Predicates::CanShootRay::compute(const World &world) {
	const FriendlyTeam &friendly = world.friendly_team();
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		const Player::CPtr player = friendly.get(i);
		if (!Evaluation::possess_ball(world, player)) {
			continue;
		}
		if (Evaluation::best_shoot_ray(world, player).first) {
			return true;
		}
	}
	return false;
}

Predicates::CanShootRay Predicates::can_shoot_ray;

bool Predicates::BallInsideRobot::compute(const World &world) {
	const Point ball = world.ball().position();

	const FriendlyTeam &friendly = world.friendly_team();
	for (std::size_t i = 0; i < friendly.size(); ++i) {
		if ((friendly.get(i)->position() - ball).len() < AI::HL::Util::POS_CLOSE) {
			return true;
		}
	}

	const EnemyTeam &enemies = world.enemy_team();
	for (std::size_t i = 0; i < enemies.size(); ++i) {
		if ((enemies.get(i)->position() - ball).len() < AI::HL::Util::POS_CLOSE) {
			return true;
		}
	}
	return false;
}

Predicates::BallInsideRobot Predicates::ball_inside_robot;

bool Predicates::EnemyBreakDefenseDuo::compute(const World &world) {
	const EnemyTeam &enemies = world.enemy_team();
	for (std::size_t i = 0; i < enemies.size(); ++i) {
		if (Evaluation::enemy_break_defense_duo(world, enemies.get(i))) {
			return true;
		}
	}
	return false;
}

Predicates::EnemyBreakDefenseDuo Predicates::enemy_break_defense_duo;

bool Predicates::BallTowardsEnemy::compute(const World &world) {
	const EnemyTeam &enemies = world.enemy_team();
	for (std::size_t i = 0; i < enemies.size(); ++i) {
		if (Evaluation::evaluate_ball_threat(world).activate_steal) {
			return true;
		}
	}

	return false;
}

Predicates::BallTowardsEnemy Predicates::ball_towards_enemy;

bool Predicates::BallOnEnemyNet::compute(const World &world) {
	return Evaluation::ball_on_enemy_net(world);
}

Predicates::BallOnEnemyNet Predicates::ball_on_enemy_net;

