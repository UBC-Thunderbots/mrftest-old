#include "ai/hl/stp/tactic/idle.h"
#include "ai/hl/stp/play/simple_play.h"

BEGIN_DEC(Idle)
INVARIANT(true)
APPLICABLE(true)
END_DEC(Idle)

BEGIN_DEF(Idle)
DONE(false)
FAIL(false)
EXECUTE()
tactics[0] = Tactic::idle(world);
tactics[1] = Tactic::idle(world);
tactics[2] = Tactic::idle(world);
tactics[3] = Tactic::idle(world);
tactics[4] = Tactic::idle(world);
tactics[5] = Tactic::idle(world);

wait(caller, tactics[0].get());
END_DEF(Idle)
