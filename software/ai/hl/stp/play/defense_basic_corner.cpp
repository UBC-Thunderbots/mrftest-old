#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/chase.h"
#include "ai/hl/stp/tactic/ball.h"
#include "ai/hl/stp/tactic/defend_solo.h"
#include "ai/hl/stp/tactic/tdefend.h"

using AI::HL::STP::Coordinate;

BEGIN_PLAY(CMDBasicCorner)

#warning LONE GOALIE
INVARIANT(false)
// INVARIANT(playtype(world, PlayType::PLAY) && our_team_size_at_least(world, 3))

APPLICABLE(defensive(world) && ball_in_our_corner(world))
DONE(offensive(world) || !ball_in_our_corner(world))
FAIL(false)
BEGIN_ASSIGN()

// GOALIE
goalie_role.push_back(lone_goalie(world));

// ROLE 1
// cm active def
// roles[0].push_back(chase(world));
roles[0].push_back(tactive_def(world));

// ROLE 2
// defend lane
roles[1].push_back(tdefend_line(world, Coordinate(world, Point(-1.35, 0.59), Coordinate::YType::BALL, Coordinate::OriginType::BALL), Coordinate(world, Point(-1.1, 0.59), Coordinate::YType::BALL, Coordinate::OriginType::BALL), 0, 0.2));

// ROLE 3 (optional)
// defend line 1
roles[2].push_back(tdefend_line(world, Coordinate(world, Point(-1.1, 0.25), Coordinate::YType::BALL, Coordinate::OriginType::BALL), Coordinate(world, Point(-0.7, 0.25), Coordinate::YType::BALL, Coordinate::OriginType::BALL), 0, 0.2));

// ROLE 4 (optional)
// defend line 2
roles[3].push_back(tdefend_line(world, Coordinate(world, Point(-1.1, 0.35), Coordinate::YType::BALL, Coordinate::OriginType::BALL), Coordinate(world, Point(-0.7, -0.35), Coordinate::YType::BALL, Coordinate::OriginType::BALL), 0, 0.2));
END_ASSIGN()
END_PLAY()
