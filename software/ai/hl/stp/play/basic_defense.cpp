#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/block_shot_path.h"
#include "ai/hl/stp/tactic/tdefend.h"

BEGIN_DEC(BasicDefense)
INVARIANT(playtype(world, PlayType::PLAY) && our_team_size_at_least(world, 3))
APPLICABLE(ball_on_our_side(world) && their_ball(world))
END_DEC(BasicDefense)

BEGIN_DEF(BasicDefense)
DONE(ball_on_their_side(world) || our_ball(world))
FAIL(false)
EXECUTE()
tactics[0] = Tactic::defend_duo_goalie(world);
tactics[1] = Tactic::tdefender1(world, true);
tactics[2] = Tactic::defend_duo_defender(world);
tactics[3] = Tactic::defend_duo_extra1(world);
tactics[4] = Tactic::defend_duo_extra2(world);
tactics[5] = Tactic::defend_duo_extra3(world);

wait(caller, tactics[1].get());
END_DEF(BasicDefense)
