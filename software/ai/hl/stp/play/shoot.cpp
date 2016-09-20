#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/idle.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/shoot.h"

BEGIN_DEC(Shoot)
INVARIANT(true)
APPLICABLE(true)
END_DEC(Shoot)

BEGIN_DEF(Shoot)
DONE(false)
FAIL(false)
EXECUTE()
tactics[0] = Tactic::idle(world);
tactics[1] = Tactic::shoot_goal(world);
tactics[2] = Tactic::idle(world);
tactics[3] = Tactic::idle(world);
tactics[4] = Tactic::idle(world);
tactics[5] = Tactic::idle(world);

wait(caller, tactics[1].get());
END_DEF(Shoot)
