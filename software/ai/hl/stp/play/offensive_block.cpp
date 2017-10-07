#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/ball.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/offend.h"
#include "ai/hl/stp/tactic/shadow_enemy.h"
#include "ai/hl/stp/tactic/shoot.h"

BEGIN_DEC(OffensiveBlock)
INVARIANT(
    playtype(world, PlayType::PLAY) && our_team_size_at_least(world, 3) &&
    !enemy_baller_can_shoot(world) && enemy_baller_can_pass(world))
APPLICABLE(their_ball(world))
END_DEC(OffensiveBlock)

BEGIN_DEF(OffensiveBlock)
DONE(!their_ball(world))
FAIL(ball_on_our_side(world))
EXECUTE()
tactics[0] = Tactic::defend_duo_goalie(world);
tactics[1] = Tactic::defend_duo_defender(world);
tactics[2] = Tactic::tactive_def(world);
tactics[3] = Tactic::block_ball(world, Enemy::closest_friendly_goal(world, 0));
tactics[4] = Tactic::block_ball(world, Enemy::closest_friendly_goal(world, 1));
tactics[5] = Tactic::defend_duo_extra1(world);

wait(caller, tactics[2].get());
END_DEF(OffensiveBlock)
