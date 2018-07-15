#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/free_kick_to_goal.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/idle.h"

BEGIN_DEC(FreeKickFriendly)
INVARIANT(playtype(world, PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY) || playtype(world, PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY))
APPLICABLE(true)
END_DEC(FreeKickFriendly)

BEGIN_DEF(FreeKickFriendly)
DONE(false)
FAIL(false)
EXECUTE()
tactics[0] = Tactic::goalie(world);
tactics[1] = Tactic::free_kick_to_goal(world);
tactics[2] = Tactic::offend(world);
tactics[3] = Tactic::offend(world);
tactics[4] = Tactic::offend_secondary(world);
tactics[5] = Tactic::goalieAssist1(world);

while (1) yield(caller);
END_DEF(FreeKickFriendly)
