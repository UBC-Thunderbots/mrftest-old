#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/legacy_offend.h"
#include "ai/hl/stp/tactic/legacy_defend.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/tactic/move_stop.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/block_shot_path.h"


BEGIN_DEC(Stop)
INVARIANT(playtype(world, PlayType::STOP) || playtype(world, PlayType::BALL_PLACEMENT_ENEMY))
APPLICABLE(true)
END_DEC(Stop)

BEGIN_DEF(Stop)
DONE(false)
FAIL(false)
EXECUTE()
tactics[0] = Tactic::move(world, Point(world.field().friendly_goal().x + 0.1, 0));
tactics[1] = Tactic::move_stop(world, 0);
tactics[2] = Tactic::move_stop(world, 1);
tactics[3] = Tactic::move_stop(world, 2);
tactics[4] = Tactic::block_shot_path(world, 0);
tactics[5] = Tactic::block_shot_path(world, 1);

while (1) yield(caller);
END_DEF(Stop)
