#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/repel.h"

namespace Predicates = AI::HL::STP::Predicates;
using AI::HL::STP::Enemy;

BEGIN_PLAY(JustRepel)
INVARIANT(our_team_size_at_least(world, 2))
APPLICABLE(Predicates::none_ball(world))
DONE(our_ball(world))
FAIL(false)
BEGIN_ASSIGN()

// GOALIE
goalie_role.push_back(goalie_dynamic(world, 1));

// ROLE 1
// repel
roles[0].push_back(repel(world));

// ROLE 2
// defend
roles[1].push_back(defend_duo_defender(world));

// ROLE 3 (optional)
// block
roles[2].push_back(block_ball(world, Enemy::closest_ball(world, 0)));

// ROLE 4 (optional)
// offensive support
roles[3].push_back(block_ball(world, Enemy::closest_ball(world, 1)));
END_ASSIGN()
END_PLAY()

