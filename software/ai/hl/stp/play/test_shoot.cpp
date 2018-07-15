//
// Created by evan on 19/05/18.
//

#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/idle.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/shoot.h"

BEGIN_DEC(SHOOT)
INVARIANT(true)
APPLICABLE(true)
END_DEC(SHOOT)

BEGIN_DEF(SHOOT)
DONE(false)
FAIL(false)
EXECUTE()

tactics[0] = Tactic::idle(world);
tactics[1] = Tactic::shoot_goal(world);
tactics[2] = Tactic::idle(world);
tactics[3] = Tactic::idle(world);
tactics[4] = Tactic::idle(world);
tactics[5] = Tactic::idle(world);

END_DEF(Shoot)