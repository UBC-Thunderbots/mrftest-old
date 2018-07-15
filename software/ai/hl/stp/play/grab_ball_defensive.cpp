#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/block_shot_path.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/shadow_enemy.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/tactic/idle.h"

BEGIN_DEC(GrabBallDefensive)
INVARIANT(playtype(world, PlayType::PLAY) && our_team_size_at_least(world, 2))
APPLICABLE(!our_ball(world))
END_DEC(GrabBallDefensive)

BEGIN_DEF(GrabBallDefensive)
DONE(our_ball(world))
FAIL(false)
EXECUTE()
tactics[0] = Tactic::goalie(world);
tactics[1] = Tactic::shoot_goal(world);
tactics[2] = Tactic::goalieAssist1(world);
tactics[3] = Tactic::goalieAssist2(world);
tactics[4] = Tactic::shadow_enemy(world, 0);
tactics[5] = Tactic::idle(world); //Tactic::block_shot_path(world, 1);

wait(caller, tactics[1].get());
END_DEF(GrabBallDefensive)
