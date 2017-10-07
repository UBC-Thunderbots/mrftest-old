#include "ai/hl//stp/tactic/defend_solo.h"
#include "ai/hl//stp/tactic/goal_line_defense.h"
#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/block_shot_path.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/tdefend.h"

BEGIN_DEC(BasicDefense)
INVARIANT(playtype(world, PlayType::PLAY) && our_team_size_at_least(world, 1))
APPLICABLE(their_ball(world))
END_DEC(BasicDefense)

BEGIN_DEF(BasicDefense)
DONE(ball_on_their_side(world) || our_ball(world))
FAIL(false)
EXECUTE()
tactics[0] = Tactic::lone_goalie(world);
tactics[1] = Tactic::tdefender1(world, true);
tactics[2] = Tactic::goal_line_defense_top(world);
tactics[3] = Tactic::goal_line_defense_bottom(world);
tactics[4] = Tactic::block_shot_path(world, 0, 1.5);
tactics[5] = Tactic::block_shot_path(world, 1, 1.5);

wait(caller, tactics[1].get());
END_DEF(BasicDefense)
