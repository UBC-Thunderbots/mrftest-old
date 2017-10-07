#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/test_tactics.h"

BEGIN_DEC(TESTCATCH)
INVARIANT(true)
APPLICABLE(true)
END_DEC(TESTCATCH)

BEGIN_DEF(TESTCATCH)
DONE(false)
FAIL(false)
EXECUTE()

tactics[0] = Tactic::move_test_orientation(world, Point(1, 1));
tactics[1] = Tactic::catch_test(world);
/*
tactics[2] = Tactic::move_test_orientation(world, Point(1, -1));
tactics[3] = Tactic::move_test_orientation(world, Point(1, -0.5));
tactics[4] = Tactic::move_test_orientation(world, Point(2, 0.5));
tactics[5] = Tactic::move_test_orientation(world, Point(1, 0));
*/
wait(caller, tactics[0].get());
// wait(caller, tactics[1].get());
// wait(caller, tactics[2].get());

END_DEF(TESTCATCH)
