#include "../tactic/defend.h"
#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/shadow_enemy.h"
#include "ai/hl/stp/tactic/shadow_kickoff.h"
#include "ai/hl/stp/tactic/block_shot_path.h"
#include "ai/hl/stp/tactic/defend_solo.h"

BEGIN_DEC(CornerKickEnemy)
INVARIANT((playtype(world, PlayType::EXECUTE_DIRECT_FREE_KICK_ENEMY) || playtype(world, PlayType::EXECUTE_INDIRECT_FREE_KICK_ENEMY)) && our_team_size_at_least(world, 2))
APPLICABLE(ball_in_our_corner(world))
END_DEC(CornerKickEnemy)

BEGIN_DEF(CornerKickEnemy)
DONE(false)
FAIL(false)
EXECUTE()
tactics[0] = Tactic::lone_goalie(world);
tactics[1] = Tactic::defend_duo_defender(world);
//tactics[2] = Tactic::defend_duo_extra1(world);
//tactics[3] = Tactic::defend_duo_extra2(world);
//tactics[4] = Tactic::defend_duo_extra3(world);
tactics[2] = Tactic::block_shot_path(world, 0, 0.7);
tactics[3] = Tactic::block_shot_path(world,1,0.7);
tactics[4] = Tactic::block_shot_path(world, 2,0.7);
tactics[5] = Tactic::defend_duo_extra1(world);


while (1) yield(caller);

END_DEF(CornerKickEnemy)
