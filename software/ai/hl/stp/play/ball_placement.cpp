#include "ai/hl/stp/tactic/ball_placement.h"
#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/move_stop.h"

BEGIN_DEC(BallPlacement)
INVARIANT(
    Predicates::playtype(world, AI::Common::PlayType::BALL_PLACEMENT_FRIENDLY))
APPLICABLE(true)
END_DEC(BallPlacement)

BEGIN_DEF(BallPlacement)
DONE(false)
FAIL(false)
EXECUTE()
tactics[0] =
    Tactic::move(world, Point(world.field().friendly_goal().x + 0.1, 0));
tactics[1] = Tactic::ball_placement(world);
tactics[2] = Tactic::move_stop(world, 1);
tactics[3] = Tactic::move_stop(world, 2);
tactics[4] = Tactic::move_stop(world, 3);
tactics[5] = Tactic::move_stop(world, 4);

while (1)
    yield(caller);
END_DEF(BallPlacement)
