#include "../tactic/ball.h"
#include "../tactic/defend.h"
#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/shadow_enemy.h"
#include "ai/hl/stp/tactic/shoot.h"
#include "ai/hl/stp/tactic/block_shot_path.h"

BEGIN_DEC(FranticDefense)
INVARIANT(playtype(world, PlayType::PLAY) && our_team_size_at_least(world, 3) && (enemy_baller_can_shoot(world) || ball_near_friendly_goal(world)))
APPLICABLE(true)
END_DEC(FranticDefense)

BEGIN_DEF(FranticDefense)
DONE(!their_ball(world) && !ball_near_friendly_goal(world))
FAIL(false)
EXECUTE()
tactics[0] = Tactic::defend_duo_goalie(world);
tactics[1] = Tactic::tactive_def(world);
tactics[2] = Tactic::defend_duo_defender(world);
tactics[3] = Tactic::block_shot_path(world, 0);
tactics[4] = Tactic::block_shot_path(world, 1);
tactics[5] = Tactic::block_shot_path(world, 2);

wait(caller, tactics[1].get());
END_DEF(FranticDefense)
