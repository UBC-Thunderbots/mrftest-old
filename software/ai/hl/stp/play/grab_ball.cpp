#include "ai/hl/stp/tactic/chase.h"
#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/block.h"

namespace Predicates = AI::HL::STP::Predicates;
using AI::HL::STP::Enemy;

BEGIN_PLAY(GrabBall)
INVARIANT(Predicates::playtype(world, AI::Common::PlayType::PLAY) && Predicates::our_team_size_at_least(world, 3))
APPLICABLE(Predicates::none_ball(world))
DONE(Predicates::our_ball(world))
FAIL(Predicates::their_ball(world))
BEGIN_ASSIGN()
// GOALIE
// defend the goal
goalie_role.push_back(defend_duo_goalie(world));

// ROLE 1
// chase the ball!
roles[0].push_back(chase(world));

// ROLE 2
// defend
roles[1].push_back(defend_duo_defender(world));

// ROLE 3 (optional)
// offensive support
roles[2].push_back(offend(world));

// ROLE 4 (optional)
// extra defense
roles[3].push_back(block(world, Enemy::closest_ball(world, 0)));
END_ASSIGN()
END_PLAY()

