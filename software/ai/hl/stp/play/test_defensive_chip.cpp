#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/test_tactics.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/tactic/defensive_chip.h"

BEGIN_DEC(TESTDEFENSIVECHIP)
INVARIANT(true)
APPLICABLE(true)
END_DEC(TESTDEFENSIVECHIP)

BEGIN_DEF(TESTDEFENSIVECHIP)
DONE(false)
FAIL(false)
EXECUTE()

tactics[0] = Tactic::move_test_orientation(world, Point(0, 0));
tactics[1] = Tactic::chip_upfield(world);

wait(caller, tactics[0].get());
wait(caller, tactics[1].get());

END_DEF(TESTDEFENSIVECHIP)
