#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/tactic/move_stop.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/wait_playtype.h"

using namespace AI::HL::STP::Play;
using namespace AI::HL::W;
namespace Predicates = AI::HL::STP::Predicates;

namespace {
	DoubleParam goalie_stop_dist("goalie stop dist", "STP/Stop", 0.09, 0.0, 1.0);
}

/**
 * Condition:
 * - It is the stop play
 *
 * Objective:
 * - Handle the stop play
 */
BEGIN_PLAY(Stop)
INVARIANT(Predicates::playtype(world, AI::Common::PlayType::STOP))
APPLICABLE(true)
DONE(false)
FAIL(false)
BEGIN_ASSIGN()
goalie_role.push_back(move(world, Point(world.field().friendly_goal().x + goalie_stop_dist, 0.0)));

// doesn't matter what the playtype we are waiting for is here, we just need an active tactic
roles[0].push_back(wait_playtype(world, move_stop(world, 0), AI::Common::PlayType::PLAY));

roles[1].push_back(move_stop(world, 1));

roles[2].push_back(move_stop(world, 2));

roles[3].push_back(move_stop(world, 3));

roles[4].push_back(move_stop(world, 4));

END_ASSIGN()
END_PLAY()

