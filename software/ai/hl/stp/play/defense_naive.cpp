#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/chase.h"
#include "ai/hl/stp/tactic/cm_ball.h"
#include "ai/hl/stp/tactic/defend_solo.h"
#include "ai/hl/stp/tactic/tdefend.h"

using AI::HL::STP::Coordinate;

BEGIN_PLAY(CMDNaive)
//INVARIANT(false)
INVARIANT(playtype(world, PlayType::PLAY) && our_team_size_at_least(world, 3) && num_of_enemies_on_our_side_at_least(world, 1))
APPLICABLE(defensive(world) && !ball_in_our_corner(world))
DONE(offensive(world))
FAIL(ball_in_our_corner(world))
BEGIN_ASSIGN()

// GOALIE
goalie_role.push_back(defend_duo_goalie(world));

// ROLE 1
// active def
roles[0].push_back(tactive_def(world));

// ROLE 2
// duo defender
roles[1].push_back(defend_duo_defender(world));

// ROLE 3 (optional)
// defend line 1
roles[2].push_back(tdefend_line(world, Coordinate(world, Point(-1.1, 0.25), Coordinate::YType::BALL, Coordinate::OriginType::BALL), Coordinate(world, Point(-0.7, 0.25), Coordinate::YType::BALL, Coordinate::OriginType::BALL), 0, 0.2));

// ROLE 4 (optional)
// defend line 2
roles[3].push_back(tdefend_line(world, Coordinate(world, Point(-1.1, 0.35), Coordinate::YType::BALL, Coordinate::OriginType::BALL), Coordinate(world, Point(-0.7, -0.35), Coordinate::YType::BALL, Coordinate::OriginType::BALL), 0, 0.2));

END_ASSIGN()
END_PLAY()

