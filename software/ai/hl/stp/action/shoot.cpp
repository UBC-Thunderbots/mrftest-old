#include "ai/hl/stp/evaluation/shoot.h"
#include "ai/hl/stp/action/catch.h"
#include "ai/hl/stp/action/shoot.h"
#include "ai/hl/stp/evaluation/ball.h"
#include "ai/hl/stp/evaluation/player.h"
#include "ai/hl/util.h"

using namespace AI::HL::STP;

void AI::HL::STP::Action::shoot_goal(caller_t& ca, World world, Player player)
{
    Point shot = Evaluation::get_best_shot(world, player);

    shoot_target(ca, world, player, shot, BALL_MAX_SPEED);
}

void AI::HL::STP::Action::shoot_target(
    caller_t& ca, World world, Player player, Point target, double velocity,
    bool chip)
{
    const Angle orient = (target - player.position()).orientation();
    const double shootPositionOvershoot = 0.05;

    // wait for chicker to be ready
    while (!player.chicker_ready())
        Action::yield(ca);

    AI::BE::Primitives::Ptr prim(new Primitives::Shoot(
        player,
        world.ball().position() +
            (world.ball().position() - player.position())
                .norm(shootPositionOvershoot),
        orient, velocity, chip));

    (static_cast<AI::Common::Player>(player)).impl->push_prim(prim);

    Action::yield(ca);
}

void AI::HL::STP::Action::catch_and_shoot_target(
    caller_t& ca, World world, Player player, Point target, double velocity,
    bool chip)
{
    catch_ball(
        ca, world, player,
        target);  // first catch the ball (only returns when the ball is caught

    shoot_target(
        ca, world, player, target, velocity, chip);  // then shoot the ball

    yield(ca);
    return;
}

void AI::HL::STP::Action::catch_and_shoot_goal(
    caller_t& ca, World world, Player player)
{
    Evaluation::ShootData shoot_data =
        Evaluation::evaluate_shoot(world, player);

    catch_and_shoot_target(
        ca, world, player, shoot_data.target, BALL_MAX_SPEED);
}
