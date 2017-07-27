#include "../tactic/defend.h"
#include "ai/hl/stp/tactic/intercept.h"
#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/shadow_enemy.h"

BEGIN_DEC(GrabBallOffensive)
INVARIANT(playtype(world, PlayType::PLAY) && our_team_size_at_least(world, 2))
APPLICABLE(none_ball(world) && ball_on_their_side(world))
END_DEC(GrabBallOffensive)

BEGIN_DEF(GrabBallOffensive)
DONE(our_ball(world))
FAIL(their_ball(world))
EXECUTE()
tactics[0] = Tactic::defend_duo_goalie(world);
tactics[1] = Tactic::shoot_goal(world);
tactics[2] = Tactic::offend(world);
tactics[3] = Tactic::offend_secondary(world);
tactics[4] = Tactic::defend_duo_defender(world);
tactics[5] = Tactic::defend_duo_extra1(world);

wait(caller, tactics[1].get());
END_DEF(GrabBallOffensive)
