#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/ball.h"
#include "ai/hl/stp/tactic/defend_solo.h"
#include "ai/hl/stp/tactic/tdefend.h"

using AI::HL::STP::Coordinate;

BEGIN_PLAY(DBasicCorner)

#warning Consider Retiring this play
INVARIANT(false)
// INVARIANT(playtype(world, PlayType::PLAY) && our_team_size_at_least(world, 3))

APPLICABLE(defensive(world) && ball_in_our_corner(world))
DONE(offensive(world) || !ball_in_our_corner(world))
FAIL(false)
BEGIN_ASSIGN()

// GOALIE
goalie_role.push_back(lone_goalie(world));

// ROLE 1
// active def
// roles[0].push_back(chase(world));
roles[0].push_back(tactive_def(world));

// ROLE 2
// defend lane
roles[1].push_back(tdefend_line(world, Coordinate(world, world.field().friendly_corner_pos() - Point(0.65, 0.41), Coordinate::YType::BALL, Coordinate::OriginType::BALL), Coordinate(world, world.field().friendly_corner_pos() - Point(0.9, 0.41), Coordinate::YType::BALL, Coordinate::OriginType::BALL), 0, 0.2));
// ROLE 3 (optional)
// defend line 1
roles[2].push_back(tdefend_line(world, Coordinate(world, world.field().friendly_corner_neg() + Point(1.1, 4.25), Coordinate::YType::BALL, Coordinate::OriginType::BALL), Coordinate(world, world.field().friendly_goal() + Point(3.70, 0.25), Coordinate::YType::BALL, Coordinate::OriginType::BALL), 0, 0.2));
// ROLE 4 (optional)
// defend line 2
roles[3].push_back(tdefend_line(world, Coordinate(world, world.field().friendly_goal() - Point(1.1, 0.35), Coordinate::YType::BALL, Coordinate::OriginType::BALL), Coordinate(world, world.field().friendly_goal() - Point(0.7, -0.35), Coordinate::YType::BALL, Coordinate::OriginType::BALL), 0, 0.2));
// ROLE 5 (optional)
// offend
//roles[4].push_back(offend(world));

END_ASSIGN()
END_PLAY()

