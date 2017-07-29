#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/defend_solo.h"
#include "ai/hl/stp/tactic/goal_line_defense.h"
#include "ai/hl/stp/tactic/block_shot_path.h"

BEGIN_DEC(DefensiveOffense)
INVARIANT(playtype(world, PlayType::PLAY))
APPLICABLE(our_ball(world))
END_DEC(DefensiveOffense)

BEGIN_DEF(DefensiveOffense)
DONE(false)
FAIL(false)
EXECUTE()
tactics[0] = Tactic::lone_goalie(world);
tactics[1] = Tactic::shoot_goal(world);
tactics[2] = Tactic::goal_line_defense_bottom(world);
tactics[3] = Tactic::goal_line_defense_top(world);
tactics[4] = Tactic::block_shot_path(world,0, 5.0);
tactics[5] = Tactic::block_shot_path(world,1, 5.0);

wait(caller, tactics[1].get());
END_DEF(DefensiveOffense)
