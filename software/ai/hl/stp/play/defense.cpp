//
// Created by evan on 19/05/18.
//

#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/idle.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/tactic/offend.h"

BEGIN_DEC(GOALIE)
INVARIANT(true)
APPLICABLE(true)
END_DEC(GOALIE)

BEGIN_DEF(GOALIE)
DONE(false)
FAIL(false)
EXECUTE()

tactics[0] = Tactic::goalie(world);
tactics[1] = Tactic::goalieAssist2(world);
tactics[2] = Tactic::goalieAssist1(world);
tactics[3] = Tactic::ballerBlocker(world);
tactics[4] = Tactic::shoot_goal(world);
tactics[5] = Tactic::offend(world);

wait(caller, tactics[1].get());

END_DEF(GOALIE)
