#include "../tactic/defend.h"
#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/block_shot_path.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/shadow_enemy.h"
#include "ai/hl/stp/tactic/shadow_kickoff.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/tactic/idle.h"

BEGIN_DEC(CornerKickEnemy)
INVARIANT(
    (playtype(world, PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY) ||
     playtype(world, PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY)) &&
    our_team_size_at_least(world, 2))
APPLICABLE(ball_in_our_corner(world))
END_DEC(CornerKickEnemy)

BEGIN_DEF(CornerKickEnemy)
DONE(false)
FAIL(false)
EXECUTE()
// one robot assigned goalie position and three assigned tactic block shot
tactics[0] = Tactic::goalie(world);
tactics[1] = Tactic::goalieAssist1(world);
tactics[2] = Tactic::goalieAssist2(world);
tactics[3] = Tactic::ballerBlocker(world); //Tactic::block_shot_path(world, 0, 0.7);
tactics[4] = Tactic::idle(world); //Tactic::block_shot_path(world, 1, 0.7);
tactics[5] = Tactic::idle(world); //Tactic::block_shot_path(world, 2, 0.7);


END_DEF(CornerKickEnemy)
