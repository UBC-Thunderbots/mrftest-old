#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/cm_defense.h"
#include "ai/hl/stp/tactic/chase.h"
#include "ai/hl/stp/tactic/cm_ball.h"
#include "ai/hl/stp/tactic/defend_solo.h"

using AI::HL::STP::Coordinate;

BEGIN_PLAY(CMONaive)

#warning LONE GOALIE
INVARIANT(false)

// INVARIANT(playtype(world, PlayType::PLAY) && our_team_size_at_least(world, 2) && baller_can_shoot(world))
APPLICABLE(offensive(world) && !ball_in_their_corner(world))
DONE(goal(world))
FAIL(defensive(world) || ball_in_their_corner(world))
BEGIN_ASSIGN()

// GOALIE
goalie_role.push_back(lone_goalie(world));

// ROLE 1
// shoot
roles[0].push_back(chase(world));
roles[0].push_back(shoot_goal(world));

// ROLE 2 (optional)
// cm defend point 1
roles[1].push_back(tdefend_point(world, Coordinate(world, Point(-1.4, 0.5), Coordinate::YType::BALL, Coordinate::OriginType::BALL), 0, 0.7));

// ROLE 3 (optional)
// cm defend point 2
roles[2].push_back(tdefend_point(world, Coordinate(world, Point(-1.4, -0.5), Coordinate::YType::BALL, Coordinate::OriginType::BALL), 0, 0.7));

// ROLE 4 (optional)
// cm defend lane
roles[3].push_back(tdefend_lane(world, Coordinate(world, Point(0, -0.2), Coordinate::YType::BALL, Coordinate::OriginType::BALL), Coordinate(world, Point(1.175, -0.2), Coordinate::YType::BALL, Coordinate::OriginType::BALL)));
END_ASSIGN()
END_PLAY()

