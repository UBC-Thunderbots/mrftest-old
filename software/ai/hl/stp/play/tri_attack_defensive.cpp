#include "ai/hl/stp/tactic/ball.h"
#include "ai/hl/stp/tactic/tactic.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/defend_solo.h"
#include "ai/hl/stp/tactic/tdefend.h"
#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/block_shot_path.h"

BEGIN_DEC(TriAttackDef)
INVARIANT(playtype(world, PlayType::PLAY) && our_team_size_at_least(world, 3) &&
		num_of_enemies_on_our_side_at_least(world, 1))
APPLICABLE(defensive(world) && !ball_in_our_corner(world))
END_DEC(TriAttackDef)

BEGIN_DEF(TriAttackDef)
DONE(offensive(world))
FAIL(ball_in_our_corner(world))
EXECUTE()
tactics[0] = Tactic::defend_duo_goalie(world);
tactics[1] = Tactic::tactive_def(world);
tactics[2] = Tactic::defend_duo_defender(world);
tactics[3] = Tactic::tdefend_line(world, Coordinate(world, Point(-1.1, 0.25), Coordinate::YType::BALL, Coordinate::OriginType::BALL), Coordinate(world, Point(-0.7, 0.25), Coordinate::YType::BALL, Coordinate::OriginType::BALL), 0, 1.5);

tactics[4] = Tactic::tdefend_line(world, Coordinate(world, Point(-1.1, 0.35), Coordinate::YType::BALL, Coordinate::OriginType::BALL), Coordinate(world, Point(-0.7, -0.35), Coordinate::YType::BALL, Coordinate::OriginType::BALL), 0, 1.5);

tactics[5] = Tactic::block_shot_path(world,0);

wait(caller, tactics[1].get());
END_DEF(TriAttackDef)
