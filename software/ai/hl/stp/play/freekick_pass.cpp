
#include "ai/hl/stp/play/simple_play.h"
//#include "ai/hl/stp/tactic/defend_solo.h"
//#include "ai/hl/stp/tactic/goal_line_defense.h"
#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/pass_simple.h"
//#include "ai/hl/stp/tactic/block_shot_path.h"
#include "ai/hl//stp/tactic/defend.h"
#include "ai/hl//stp/tactic/shadow_enemy.h"


BEGIN_DEC(FreeKickPass)
INVARIANT(our_team_size_at_least(world, 3)  && their_team_size_at_least(world, 2) && ((playtype(world, PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY) || playtype(world, PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY)||playtype(world, PlayType::PLAY))))
APPLICABLE((playtype(world, PlayType::EXECUTE_DIRECT_FREE_KICK_FRIENDLY) || playtype(world, PlayType::EXECUTE_INDIRECT_FREE_KICK_FRIENDLY)))
END_DEC(FreeKickPass)
BEGIN_DEF(FreeKickPass)
DONE(false)
FAIL(false)
EXECUTE()
// STEP 1
tactics[0] = Tactic::goalie(world);
tactics[1] = Tactic::passer_simple(world, true);
tactics[2] = Tactic::passee_simple(world, 0);
//tactics[3] = Tactic::passee_simple(world, 1);
tactics[3] = Tactic::goalieAssist1(world);
tactics[4] = Tactic::goalieAssist2(world);
tactics[5] = Tactic::shadow_enemy(world,0);
wait(caller, tactics[1].get());

// STEP 2
tactics[0] = Tactic::goalie(world);
tactics[1] = Tactic::passee_simple_receive(world);
tactics[2] = Tactic::passee_simple(world, 0);
tactics[3] = Tactic::goalieAssist1(world);
tactics[4] = Tactic::goalieAssist2(world);
tactics[5] = Tactic::shadow_enemy(world,0);
wait(caller, tactics[1].get());
END_DEF(FreeKickPass)
