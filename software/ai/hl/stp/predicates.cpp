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
#include <cstdlib>

using namespace AI::HL::STP;

namespace {
	/* shoot_anyway should be adjusted when facing different teams. */
        IntParam shoot_anyway(u8"randomize factor that the baller will shoot even if blocked", u8"STP/predicates", 5, 1, 10);
	DoubleParam near_thresh(u8"enemy avoidance distance (robot radius)", u8"STP/predicates", 3.0, 1.0, 10.0);
	DoubleParam fight_thresh(u8"dist thresh to start fight ball with enemy (robot radius)", u8"STP/predicates", 2.0, 0.1, 4.0);

	BoolParam new_fight(u8"new fight", u8"STP/predicates", true);
}

bool Predicates::Goal::compute(World) {
	return false;
}

Predicates::Goal Predicates::goal;

bool Predicates::Playtype::compute(World world, AI::Common::PlayType playtype) {
	return world.playtype() == playtype;
}

Predicates::Playtype Predicates::playtype;

bool Predicates::OurBall::compute(World world) {
	for (const Player i : world.friendly_team()) {
		if (Evaluation::possess_ball(world, i)) {
			return true;
		}
	}
	return false;
}

Predicates::OurBall Predicates::our_ball;

bool Predicates::TheirBall::compute(World world) {
	for (const Robot i : world.enemy_team()) {
		if (Evaluation::possess_ball(world, i)) {
			return true;
		}
	}
	return false;
}

Predicates::TheirBall Predicates::their_ball;

bool Predicates::NoneBall::compute(World world) {
	return !our_ball(world) && !their_ball(world);
}

Predicates::NoneBall Predicates::none_ball;

bool Predicates::OurTeamSizeAtLeast::compute(World world, const unsigned int n) {
	return world.friendly_team().size() >= n;
}

Predicates::OurTeamSizeAtLeast Predicates::our_team_size_at_least;

bool Predicates::OurTeamSizeExactly::compute(World world, const unsigned int n) {
	return world.friendly_team().size() == n;
}

Predicates::OurTeamSizeExactly Predicates::our_team_size_exactly;

bool Predicates::TheirTeamSizeAtLeast::compute(World world, const unsigned int n) {
	return world.enemy_team().size() >= n;
}

Predicates::TheirTeamSizeAtLeast Predicates::their_team_size_at_least;

bool Predicates::TheirTeamSizeAtMost::compute(World world, const unsigned int n) {
	return world.enemy_team().size() <= n;
}

Predicates::TheirTeamSizeAtMost Predicates::their_team_size_at_most;

bool Predicates::BallXLessThan::compute(World world, const double x) {
	return world.ball().position().x < x;
}

Predicates::BallXLessThan Predicates::ball_x_less_than;

bool Predicates::BallXGreaterThan::compute(World world, const double x) {
	return world.ball().position().x > x;
}

Predicates::BallXGreaterThan Predicates::ball_x_greater_than;

bool Predicates::BallOnOurSide::compute(World world) {
	return world.ball().position().x <= 0;
}

Predicates::BallOnOurSide Predicates::ball_on_our_side;

bool Predicates::BallOnTheirSide::compute(World world) {
	return world.ball().position().x > 0;
}

Predicates::BallOnTheirSide Predicates::ball_on_their_side;

bool Predicates::BallInOurCorner::compute(World world) {
	return world.ball().position().x <= -world.field().length() / 4 && std::fabs(world.ball().position().y) > world.field().goal_width() / 2 + world.field().defense_area_radius();
}

Predicates::BallInOurCorner Predicates::ball_in_our_corner;

bool Predicates::BallInTheirCorner::compute(World world) {
	return world.ball().position().x >= world.field().length() / 4 && std::fabs(world.ball().position().y) > world.field().goal_width() / 2 + world.field().defense_area_radius();
}

Predicates::BallInTheirCorner Predicates::ball_in_their_corner;

bool Predicates::BallMidfield::compute(World world) {
	return std::fabs(world.ball().position().x) < world.field().length() / 4;
}

Predicates::BallMidfield Predicates::ball_midfield;

bool Predicates::BallNearFriendlyGoal::compute(World world) {
	return ball_on_our_side(world) && !ball_in_our_corner(world) && !ball_midfield(world);
}

Predicates::BallNearFriendlyGoal Predicates::ball_near_friendly_goal;

bool Predicates::BallNearEnemyGoal::compute(World world) {
	return ball_on_their_side(world) && !ball_in_their_corner(world) && !ball_midfield(world);
}

Predicates::BallNearEnemyGoal Predicates::ball_near_enemy_goal;

bool Predicates::BallerCanShoot::compute(World world) {
	const Player baller = Evaluation::calc_friendly_baller();
	if (!baller || !Evaluation::possess_ball(world, baller)) {
		return false;
	}
	if (shoot_anyway > std::rand()%10) return true;
	return Evaluation::evaluate_shoot(world, baller).angle >= min_shoot_region;
}

Predicates::BallerCanShoot Predicates::baller_can_shoot;

bool Predicates::BallerCanChip::compute(World world) {
	const Player baller = Evaluation::calc_friendly_baller();
	if (!baller || !Evaluation::possess_ball(world, baller) || !baller.has_chipper()) {
		return false;
	}
	
	if (shoot_anyway > std::rand()%10) return true;

	for (const Robot i : world.enemy_team()) {
		if ((baller.position() - i.position()).len() <= near_thresh * AI::HL::W::Robot::MAX_RADIUS) {
			return false;
		}
	}

	return true;
}

Predicates::BallerCanChip Predicates::baller_can_chip;

bool Predicates::BallerCanPassTarget::compute(World world, const Point target) {
	const Player baller = Evaluation::calc_friendly_baller();
	if (!baller || !Evaluation::possess_ball(world, baller)) {
		return false;
	}
#warning something is wierd
	if (shoot_anyway > std::rand()%10) return true;
	return Evaluation::can_pass(world, baller.position(), target);
}

Predicates::BallerCanPassTarget Predicates::baller_can_pass_target;

bool Predicates::BallerCanPass::compute(World world) {
	const Player baller = Evaluation::calc_friendly_baller();
	if (!baller || !Evaluation::possess_ball(world, baller)) {
		return false;
	}
	return !!Evaluation::select_passee(world);
}

Predicates::BallerCanPass Predicates::baller_can_pass;

bool Predicates::BallerUnderThreat::compute(World world) {
	const Player baller = Evaluation::calc_friendly_baller();
	if (!baller || !Evaluation::possess_ball(world, baller)) {
		return false;
	}
	int enemy_cnt = 0;
	for (const Robot i : world.enemy_team()) {
		if ((baller.position() - i.position()).len() <= near_thresh * AI::HL::W::Robot::MAX_RADIUS) {
			enemy_cnt++;
		}
	}

	return enemy_cnt >= 2;
}

Predicates::BallerUnderThreat Predicates::baller_under_threat;

bool Predicates::EnemyBallerCanShoot::compute(World world) {
	const Robot baller = Evaluation::calc_enemy_baller(world);
	if (!baller || !Evaluation::possess_ball(world, baller)) {
		return false;
	}
	return Evaluation::enemy_can_shoot_goal(world, baller);
}

Predicates::EnemyBallerCanShoot Predicates::enemy_baller_can_shoot;

bool Predicates::EnemyBallerCanPass::compute(World world) {
	const Robot baller = Evaluation::calc_enemy_baller(world);
	if (!baller || !Evaluation::possess_ball(world, baller)) {
		return false;
	}
	return Evaluation::calc_enemy_pass(world, baller) > 0;
}

Predicates::EnemyBallerCanPass Predicates::enemy_baller_can_pass;

bool Predicates::EnemyBallerCanPassShoot::compute(World world) {
	const Robot baller = Evaluation::calc_enemy_baller(world);
	if (!baller || !Evaluation::possess_ball(world, baller)) {
		return false;
	}
	return Evaluation::calc_enemy_pass(world, baller) > 0 && Evaluation::calc_enemy_pass(world, baller) < 3;
}

Predicates::EnemyBallerCanPassShoot Predicates::enemy_baller_can_pass_shoot;

bool Predicates::Offensive::compute(World world) {
	return our_ball(world) || ball_on_their_side(world);
}

Predicates::Offensive Predicates::offensive;

bool Predicates::Defensive::compute(World world) {
	return (their_ball(world) || ball_on_our_side(world)) && !offensive(world);
}

Predicates::Defensive Predicates::defensive;

bool Predicates::NumOfEnemiesOnOurSideAtLeast::compute(World world, const unsigned int n) {
	unsigned int cnt = 0;
	for (const Robot i : world.enemy_team()) {
		if (i.position().x < 0) {
			cnt++;
		}
	}
	if (cnt >= n) {
		return true;
	}
	return false;
}

Predicates::NumOfEnemiesOnOurSideAtLeast Predicates::num_of_enemies_on_our_side_at_least;

bool Predicates::BallInsideRegion::compute(World world, Region region) {
	return region.inside(world.ball().position());
}

Predicates::BallInsideRegion Predicates::ball_inside_region;

bool Predicates::FightBall::compute(World world) {
	if (!new_fight) {
		return our_ball(world) && their_ball(world);
	} else {
		const Player friendly_baller = Evaluation::calc_friendly_baller();
		const Robot enemy_baller = Evaluation::calc_enemy_baller(world);

		return (friendly_baller && enemy_baller)
		       && (friendly_baller.position() - world.ball().position()).len() < fight_thresh * Robot::MAX_RADIUS
		       && (enemy_baller.position() - world.ball().position()).len() < fight_thresh * Robot::MAX_RADIUS;
	}
}

Predicates::FightBall Predicates::fight_ball;

bool Predicates::CanShootRay::compute(World world) {
	for (const Player player : world.friendly_team()) {
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

bool Predicates::BallInsideRobot::compute(World world) {
	const Point ball = world.ball().position();

	for (const Player i : world.friendly_team()) {
		if ((i.position() - ball).len() < AI::HL::Util::POS_CLOSE) {
			return true;
		}
	}

	for (const Robot i : world.enemy_team()) {
		if ((i.position() - ball).len() < AI::HL::Util::POS_CLOSE) {
			return true;
		}
	}
	return false;
}

Predicates::BallInsideRobot Predicates::ball_inside_robot;

bool Predicates::EnemyBreakDefenseDuo::compute(World world) {
	for (const Robot i : world.enemy_team()) {
		if (Evaluation::enemy_break_defense_duo(world, i)) {
			return true;
		}
	}
	return false;
}

Predicates::EnemyBreakDefenseDuo Predicates::enemy_break_defense_duo;

bool Predicates::BallTowardsEnemy::compute(World world) {
	if (Evaluation::evaluate_ball_threat(world).activate_steal) {
		return true;
	}

	return false;
}

Predicates::BallTowardsEnemy Predicates::ball_towards_enemy;

bool Predicates::BallOnEnemyNet::compute(World world) {
	return Evaluation::ball_on_enemy_net(world);
}

Predicates::BallOnEnemyNet Predicates::ball_on_enemy_net;

