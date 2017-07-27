#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/move_stop.h"
#include "ai/hl/stp/tactic/block_shot_stop.h"


BEGIN_DEC(Stop)
INVARIANT(playtype(world, PlayType::STOP) || playtype(world, PlayType::BALL_PLACEMENT_ENEMY))
APPLICABLE(true)
END_DEC(Stop)

BEGIN_DEF(Stop)
DONE(false)
FAIL(false)
EXECUTE()
tactics[0] = Tactic::move(world, Point(world.field().friendly_goal().x + 0.35, 0));
tactics[1] = Tactic::move_stop(world, 0);
tactics[2] = Tactic::move_stop(world, 1);
tactics[3] = Tactic::move_stop(world, 2);
tactics[4] = Tactic::move_stop(world, 3);
tactics[5] = Tactic::move_stop(world, 4);
//tactics[4] = Tactic::block_shot_stop(world);
//tactics[5] = Tactic::block_shot_stop(world);

while (1) yield(caller);
END_DEF(Stop)
