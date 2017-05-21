#include "../tactic/cshoot.h"
#include "../tactic/defend.h"
#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/shoot.h"

BEGIN_DEC(JustCShoot)
INVARIANT(playtype(world, PlayType::PLAY) && our_team_size_at_least(world, 2))
APPLICABLE(our_ball(world))
END_DEC(JustCShoot)

BEGIN_DEF(JustCShoot)
DONE(goal(world))
FAIL(their_ball(world))
EXECUTE()
tactics[0] = Tactic::goalie_dynamic(world, 1);
tactics[1] = Tactic::cshoot_goal(world);
tactics[2] = Tactic::defend_duo_defender(world, true);
tactics[3] = Tactic::defend_duo_extra1(world, true);
tactics[4] = Tactic::offend(world);
tactics[5] = Tactic::offend_secondary(world);

wait(caller, tactics[1].get());
END_DEF(JustCShoot)
