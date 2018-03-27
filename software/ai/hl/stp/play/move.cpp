#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/play/simple_play.h"

BEGIN_DEC(Move)
INVARIANT(true)
APPLICABLE(true)
END_DEC(Move)

BEGIN_DEF(Move)
DONE(false)
FAIL(false)
EXECUTE()
tactics[0] = Tactic::move(world, world.ball().position());
// tactics[1] = Tactic::idle(world);
// tactics[2] = Tactic::idle(world);
// tactics[3] = Tactic::idle(world);
// tactics[4] = Tactic::idle(world);
// tactics[5] = Tactic::idle(world);

wait(caller, tactics[0].get());
END_DEF(Move)
