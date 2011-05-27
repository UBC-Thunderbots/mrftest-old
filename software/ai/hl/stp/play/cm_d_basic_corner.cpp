#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/cm_defense.h"
#include "ai/hl/stp/tactic/chase.h"
#include "ai/hl/stp/cm_coordinate.h"

using AI::HL::STP::TCoordinate;

BEGIN_PLAY(CMDBasicCorner)
INVARIANT(false) //INVARIANT(playtype(world, PlayType::PLAY) && our_team_size_at_least(world, 3))
APPLICABLE(defensive(world) && ball_in_our_corner(world))
DONE(offensive(world) || !ball_in_our_corner(world))
FAIL(false)
BEGIN_ASSIGN()
// GOALIE
goalie_role.push_back(defend_duo_goalie(world));

// ROLE 1
// shoot
roles[0].push_back(chase(world)); // should use cm active def when it's done

// ROLE 2
// cm defend lane
roles[1].push_back(tdefend_lane(world, TCoordinate(Point(-1.35, 0.59), TCoordinate::SType::BALL, TCoordinate::OType::BALL), TCoordinate(Point(-1.1, 0.59), TCoordinate::SType::BALL, TCoordinate::OType::BALL)));

// ROLE 3 (optional)
// cm defend line 1
roles[2].push_back(tdefend_line(world, TCoordinate(Point(-1.1, 0.25), TCoordinate::SType::BALL, TCoordinate::OType::BALL), TCoordinate(Point(-0.7, 0.25), TCoordinate::SType::BALL, TCoordinate::OType::BALL), 0, 0.2));

// ROLE 4 (optional)
// cm defend line 2
roles[3].push_back(tdefend_line(world, TCoordinate(Point(-1.1, 0.35), TCoordinate::SType::BALL, TCoordinate::OType::BALL), TCoordinate(Point(-0.7, -0.35), TCoordinate::SType::BALL, TCoordinate::OType::BALL), 0, 0.2));
END_ASSIGN()
END_PLAY()

