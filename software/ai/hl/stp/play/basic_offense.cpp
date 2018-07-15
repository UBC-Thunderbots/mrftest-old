#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/shoot.h"

BEGIN_DEC(BasicOffense)
INVARIANT(playtype(world, PlayType::PLAY) && our_team_size_at_least(world, 2))
APPLICABLE(our_ball(world))
END_DEC(BasicOffense)

BEGIN_DEF(BasicOffense)
DONE(!our_ball(world))
FAIL(their_ball(world))
EXECUTE()
tactics[0] = Tactic::goalie(world);
tactics[1] = Tactic::shoot_goal(world);
tactics[2] = Tactic::offend(world);
tactics[3] = Tactic::offend_secondary(world);
tactics[4] = Tactic::goalieAssist1(world);
tactics[5] = Tactic::goalieAssist2(world);

wait(caller, tactics[1].get());
END_DEF(BasicOffense)
