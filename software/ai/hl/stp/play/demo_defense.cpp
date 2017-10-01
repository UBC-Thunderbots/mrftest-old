#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/block_shot_path.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/defend_solo.h"
#include "ai/hl/stp/tactic/goal_line_defense.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/shoot.h"

BEGIN_DEC(DemoDefense)
INVARIANT(playtype(world, PlayType::PLAY))
APPLICABLE(true)
END_DEC(DemoDefense)

BEGIN_DEF(DemoDefense)
DONE(false)
FAIL(false)
EXECUTE()
tactics[0] = Tactic::lone_goalie(world);
tactics[1] = Tactic::goal_line_defense_bottom(world);
tactics[2] = Tactic::goal_line_defense_top(world);
tactics[3] = Tactic::shoot_goal(world);

wait(caller, tactics[1].get());
END_DEF(DemoDefense)
