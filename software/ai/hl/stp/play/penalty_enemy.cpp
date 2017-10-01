#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/penalty_goalie_new.h"

BEGIN_DEC(PenaltyEnemy)
INVARIANT(
    playtype(world, PlayType::PREPARE_PENALTY_ENEMY) ||
    playtype(world, PlayType::EXECUTE_PENALTY_ENEMY))
APPLICABLE(true)
END_DEC(PenaltyEnemy)

BEGIN_DEF(PenaltyEnemy)
DONE(false)
FAIL(false)
EXECUTE()
tactics[0] = Tactic::penalty_goalie_new(world);

tactics[1] = Tactic::move(
    world, Point(
               world.field().penalty_friendly().x + DIST_FROM_PENALTY_MARK +
                   Robot::MAX_RADIUS,
               7 * Robot::MAX_RADIUS));

tactics[2] = Tactic::move(
    world, Point(
               world.field().penalty_friendly().x + DIST_FROM_PENALTY_MARK +
                   Robot::MAX_RADIUS,
               3.5 * Robot::MAX_RADIUS));

tactics[3] = Tactic::move(
    world, Point(
               world.field().penalty_friendly().x + DIST_FROM_PENALTY_MARK +
                   Robot::MAX_RADIUS,
               -3.5 * Robot::MAX_RADIUS));

tactics[4] = Tactic::move(
    world, Point(
               world.field().penalty_friendly().x + DIST_FROM_PENALTY_MARK +
                   Robot::MAX_RADIUS,
               -7 * Robot::MAX_RADIUS));

tactics[5] = Tactic::move(
    world, Point(
               world.field().penalty_friendly().x + DIST_FROM_PENALTY_MARK +
                   Robot::MAX_RADIUS,
               0));

while (1)
    yield(caller);
END_DEF(PenaltyEnemy)
