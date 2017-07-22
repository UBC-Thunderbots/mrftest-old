#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/test_tactics.h"

BEGIN_DEC(TESTSHOOT)
INVARIANT(true)
APPLICABLE(true)
END_DEC(TESTSHOOT)

BEGIN_DEF(TESTSHOOT)
DONE(false)
FAIL(false)
EXECUTE()

tactics[0] = Tactic::shoot_test(world);
/* tactics[1] = Tactic::shoot_test(world, Point(0, -0.3)); */
/*tactics[1] = Tactic::move_test_orientation(world, Point(1,  1));
tactics[2] = Tactic::move_test_orientation(world, Point(1, -1));
tactics[3] = Tactic::move_test_orientation(world, Point(1, -0.5));
tactics[4] = Tactic::move_test_orientation(world, Point(2, 0.5));
tactics[5] = Tactic::move_test_orientation(world, Point(1, 0));
*/
wait(caller, tactics[0].get());
/* wait(caller, tactics[1].get()); */
//wait(caller, tactics[2].get());

END_DEF(TESTSHOOT)
