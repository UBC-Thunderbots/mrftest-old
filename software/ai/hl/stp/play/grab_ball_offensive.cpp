#include "../tactic/defend.h"
#include "ai/hl/stp/tactic/intercept.h"
#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/shadow_enemy.h"

BEGIN_DEC(GrabBallOffensive)
INVARIANT(playtype(world, PlayType::PLAY) && our_team_size_at_least(world, 2))
APPLICABLE(none_ball(world))
END_DEC(GrabBallOffensive)

BEGIN_DEF(GrabBallOffensive)
DONE(our_ball(world))
FAIL(their_ball(world))
EXECUTE()
tactics[0] = Tactic::goalie_dynamic(world, 1);
tactics[1] = Tactic::shoot_goal(world);
tactics[2] = Tactic::defend_duo_defender(world);
tactics[3] = Tactic::defend_duo_extra1(world);
tactics[4] = Tactic::offend_secondary(world);
tactics[5] = Tactic::defend_duo_extra2(world);

wait(caller, tactics[1].get());
END_DEF(GrabBallOffensive)
