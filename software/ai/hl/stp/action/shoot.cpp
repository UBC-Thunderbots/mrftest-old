#include "ai/flags.h"
#include "ai/hl/util.h"
#include "geom/point.h"
#include "geom/angle.h"
#include "geom/util.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/player.h"
#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/action/catch.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/util.h"
#include "move.h"

using namespace AI::HL::STP;

bool check_override_conditions(World world, Player player) {
    const Field& field = world.field();
    Rect friendlyGoal = field.friendly_crease();
    Rect enemyGoal = field.enemy_crease();
    Point ball = world.ball().position();
    return !friendlyGoal.point_inside(ball) && !enemyGoal.point_inside(ball) && player.velocity().len() <= 0.5;
}

void AI::HL::STP::Action::get_behind_ball(caller_t& ca, World world, Player player, Point target)
{
    double behinddist = Robot::MAX_RADIUS + 0.18;
    Point behind_ball = world.ball().position() + (world.ball().position() - target).norm(behinddist);

    Point dest;
    Angle orientation = (target - world.ball().position()).orientation();
    if (Geom::dist(Geom::Seg(player.position(), behind_ball), world.ball().position()) > Robot::MAX_RADIUS + 0.04)
    {
        dest = behind_ball;
    }
    else
    {
        Angle behindAngle = (world.ball().position() - target).orientation();
        Angle robotAngle = (player.position() - world.ball().position()).orientation();
        Angle diff = (behindAngle - robotAngle).angle_mod();
        Angle midAngle = (diff / 2.0 + robotAngle);
        dest = world.ball().position() + Point::of_angle(midAngle).norm(behinddist + 0.1);
    }

    if (check_override_conditions(world, player)) {
        Action::move_overrider(ca, world, player, dest, orientation);
    } else {
        Action::move(ca, world, player, dest, orientation);
    }
}


void AI::HL::STP::Action::shoot_target(caller_t& ca, World world, Player player, Point target, double velocity, bool chip) {
    AI::Flags::MoveFlags playerFlags = player.flags();
    //player.unset_flags(AI::Flags::MoveFlags::AVOID_BALL_MEDIUM | AI::Flags::MoveFlags::AVOID_BALL_TINY);
    if (!Evaluation::in_shoot_position(world, player, target)) {
        // Get behind the ball without hitting it
        // TODO: account for slowly moving ball (fast ball handled by catch)
        //	player.set_flags(playerFlags);
        get_behind_ball(ca, world, player, target);
    } else {
        const Angle orient = (target - player.position()).orientation();
        AI::BE::Primitives::Ptr prim(new Primitives::Shoot(player, world.ball().position(),
            orient, velocity, chip));
        prim->overrideNavigator = check_override_conditions(world, player);
        //TODO: check that this doesn't crash into things
        (static_cast<AI::Common::Player>(player)).impl->push_prim(prim);
    }
}

        // TODO: delete. we need flags to not break rules
//        player.unset_flags((AI::Flags::MoveFlags)~0);
void AI::HL::STP::Action::catch_and_shoot_target(
    caller_t& ca, World world, Player player, Point target, double velocity,
    bool chip)
{
    /*catch_ball(
        ca, world, player,
        target);  // first catch the ball (only returns when the ball is caught
		// Get behind the ball (relative to the target)
    */
    shoot_target(
        ca, world, player, target, velocity, chip);  // then shoot the ball
}


void AI::HL::STP::Action::shoot_goal(caller_t& ca, World world, Player player, bool chip) {
	Point shot = Evaluation::get_best_shot(world, player);

    printf("Shooting target");
	shoot_target(ca, world, player, shot, BALL_MAX_SPEED, chip);
}



void AI::HL::STP::Action::catch_and_shoot_goal(
    caller_t& ca, World world, Player player)
{
    Evaluation::ShootData shoot_data =
        Evaluation::evaluate_shoot(world, player);

    catch_and_shoot_target(
        ca, world, player, shoot_data.target, BALL_MAX_SPEED);
}
