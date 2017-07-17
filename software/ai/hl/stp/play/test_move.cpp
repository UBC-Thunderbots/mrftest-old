#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/test_tactics.h"

BEGIN_DEC(TESTMOVE)
INVARIANT(true)
APPLICABLE(true)
END_DEC(TESTMOVE)

BEGIN_DEF(TESTMOVE)
DONE(false)
FAIL(false)
EXECUTE()

tactics[0] = Tactic::move_test_orientation(world, Point(0, 0));
tactics[1] = Tactic::move_test_orientation(world, Point(1,  1));
tactics[2] = Tactic::move_test_orientation(world, Point(1, -1));

wait(caller, tactics[0].get());
wait(caller, tactics[1].get());
wait(caller, tactics[2].get());

END_DEF(TESTMOVE)
