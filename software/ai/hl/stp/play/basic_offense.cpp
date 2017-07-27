#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/tactic/offend.h"

BEGIN_DEC(BasicOffense)
INVARIANT(true)
APPLICABLE(true)
END_DEC(BasicOffense)

BEGIN_DEF(BasicOffense)
DONE(false)
FAIL(false)
EXECUTE()
tactics[0] = Tactic::defend_duo_goalie(world);
tactics[1] = Tactic::shoot_goal(world);
tactics[2] = Tactic::offend(world);
tactics[3] = Tactic::offend_secondary(world);
tactics[4] = Tactic::defend_duo_defender(world);
tactics[5] = Tactic::defend_duo_extra1(world);


wait(caller, tactics[1].get());
END_DEF(BasicOffense)
