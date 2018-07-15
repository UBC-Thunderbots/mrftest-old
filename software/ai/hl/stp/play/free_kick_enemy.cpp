#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/shadow_enemy.h"
#include "ai/hl/stp/tactic/shadow_kickoff.h"
#include "ai/hl/stp/tactic/block_shot_path.h"
#include "ai/hl/stp/tactic/idle.h"

BEGIN_DEC(FreeKickEnemy)
INVARIANT((playtype(world, PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY) || playtype(world, PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY)))

APPLICABLE(true)
END_DEC(FreeKickEnemy)

BEGIN_DEF(FreeKickEnemy)
DONE(false)
FAIL(false)
EXECUTE()
tactics[0] = Tactic::goalie(world);
tactics[1] = Tactic::goalieAssist1(world);
tactics[2] = Tactic::goalieAssist2(world);
// TODO: change to pass blocker
tactics[3] = Tactic::shadow_enemy(world,1);
tactics[4] = Tactic::block_shot_path(world, 0);
tactics[5] = Tactic::block_shot_path(world ,1);

while (1) yield(caller);

END_DEF(FreeKickEnemy)
