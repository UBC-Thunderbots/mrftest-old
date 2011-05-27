#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/movement_benchmark.h"
#include "ai/hl/stp/play/simple_play.h"

namespace Predicates = AI::HL::STP::Predicates;

/**
 * Condition:
 * - ball not under any possesion
 * - at least 1 players
 *
 * Objective:
 * - movement benchmark
 */
BEGIN_PLAY(MovementBenchmark)
INVARIANT(Predicates::playtype(world, AI::Common::PlayType::PLAY) && Predicates::our_team_size_at_least(world, 1))
APPLICABLE(false)
DONE(Predicates::playtype(world, AI::Common::PlayType::STOP))
FAIL(false)
BEGIN_ASSIGN()
// GOALIE
// movement benchmark
goalie_role.push_back(movement_benchmark(world));

// ROLE 1
// obstacle
roles[0].push_back(move(world, Point(0.5, 0)));

// ROLE 2
// obstacle
roles[1].push_back(move(world, Point(-0.5, 0)));

// ROLE 3 (optional)
// obstacle
roles[2].push_back(move(world, Point(0, 0.6)));

// ROLE 4 (optional)
// obstacle
roles[3].push_back(move(world, Point(0, -0.6)));
END_ASSIGN()
END_PLAY()
