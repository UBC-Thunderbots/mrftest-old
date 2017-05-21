#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/intercept.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/shadow_enemy.h"
#include "ai/hl/stp/tactic/block_shot_path.h"

BEGIN_DEC(GrabBallDefensive)
INVARIANT(playtype(world, PlayType::PLAY) && our_team_size_at_least(world, 2))
APPLICABLE(none_ball(world) && ball_x_less_than(world, 0))
END_DEC(GrabBallDefensive)

BEGIN_DEF(GrabBallDefensive)
DONE(our_ball(world))
FAIL(their_ball(world))
EXECUTE()
tactics[0] = Tactic::goalie_dynamic(world, 1);
tactics[1] = Tactic::intercept(world);
tactics[2] = Tactic::defend_duo_defender(world);
tactics[3] = Tactic::defend_duo_extra1(world);
tactics[4] = Tactic::block_shot_path(world, 0);
tactics[5] = Tactic::block_shot_path(world, 1);

wait(caller, tactics[1].get());
END_DEF(GrabBallDefensive)
