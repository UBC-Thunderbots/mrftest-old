#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/intercept_v2.h"

BEGIN_DEC(Intercept_v2_test)
INVARIANT(true)
APPLICABLE(true)
END_DEC(Intercept_v2_test)

BEGIN_DEF(Intercept_v2_test)
DONE(false)
FAIL(false)
EXECUTE()
tactics[0] = Tactic::intercept_v2(world);
tactics[1] = Tactic::move(world, Point(-1.5, 0));
tactics[2] = Tactic::move(world, Point(-3, 5));
tactics[3] = Tactic::move(world, Point(-3, 5));
tactics[4] = Tactic::move(world, Point(-3, 5));
tactics[5] = Tactic::move(world, Point(-3, 5));

wait(caller, tactics[0].get());
END_DEF(Intercept_v2_test)
