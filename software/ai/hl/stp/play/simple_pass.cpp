
#include "ai/hl/stp/play/simple_play.h"
//#include "ai/hl/stp/tactic/defend_solo.h"
//#include "ai/hl/stp/tactic/goal_line_defense.h"
#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/shadow_enemy.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/pass_simple.h"
//#include "ai/hl/stp/tactic/block_shot_path.h"
#include "ai/hl//stp/tactic/defend.h"


BEGIN_DEC(PassSimple)
INVARIANT(our_team_size_at_least(world, 3) && their_team_size_at_least(world, 2)  && playtype(world, PlayType::PLAY))
APPLICABLE(our_ball(world))
END_DEC(PassSimple)
BEGIN_DEF(PassSimple)
DONE(false)
FAIL(their_ball(world))
EXECUTE()
    //TODO: delete hack
// STEP 1
tactics[0] = Tactic::goalie(world);
tactics[1] = Tactic::passer_simple(world, true);
tactics[2] = Tactic::passee_simple(world, 0);
tactics[3] = Tactic::goalieAssist1(world);
tactics[4] = Tactic::goalieAssist2(world);
tactics[5] = Tactic::shadow_baller(world);
wait(caller, tactics[1].get());

// STEP 2
tactics[0] = Tactic::goalie(world);
tactics[1] = Tactic::passee_simple_receive(world);
tactics[2] = Tactic::passee_simple(world, 0);
tactics[3] = Tactic::goalieAssist1(world);
tactics[4] = Tactic::goalieAssist2(world);
tactics[5] = Tactic::shadow_baller(world);
wait(caller, tactics[1].get());
END_DEF(PassSimple)
