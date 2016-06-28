#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/tactic/move_stop.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/ball_placement.h"
#include "ai/hl/stp/tactic/wait_playtype.h"

using namespace AI::HL::STP::Play;
using namespace AI::HL::W;
namespace Predicates = AI::HL::STP::Predicates;

BEGIN_PLAY(BallPlacement)
INVARIANT(Predicates::playtype(world, AI::Common::PlayType::BALL_PLACEMENT_FRIENDLY))
APPLICABLE(true)
DONE(false)
FAIL(false)
BEGIN_ASSIGN()

goalie_role.push_back(goalie_move(world, Point(world.field().friendly_goal().x + 0.1, 0.0)));

// doesn't matter what the playtype we are waiting for is here, we just need an active tactic
roles[0].push_back(ball_placement(world));

roles[1].push_back(move_stop(world, 1));

roles[2].push_back(move_stop(world, 2));

roles[3].push_back(move_stop(world, 3));

roles[4].push_back(move_stop(world, 4));

END_ASSIGN()
END_PLAY()

