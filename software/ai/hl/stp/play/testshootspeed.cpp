#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/tactic/move_stop.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/wait_playtype.h"
#include "ai/hl/stp/tactic/testshootspeed.h"
#include "ai/hl/stp/tactic/shoot.h"
//#include "ai/hl/stp/play/testshootspeed.h"

using namespace AI::HL::STP::Play;
using namespace AI::HL::W;
namespace Predicates = AI::HL::STP::Predicates;

/**
 * Condition:
 * - It is the stop play
 *
 * Objective:
 * - Handle the stop play
 */
BEGIN_PLAY(TestShootSpeed)
INVARIANT(Predicates::playtype(world, AI::Common::PlayType::PLAY))
APPLICABLE(true)
DONE(false)
FAIL(false)
BEGIN_ASSIGN()
goalie_role.push_back(move(world, Point(world.field().friendly_goal().x, 0.0)));

// doesn't matter what the playtype we are waiting for is here, we just need an active tactic
roles[0].push_back(test_shoot_speed(world, true));

roles[1].push_back(move(world, Point (0,0)));

roles[2].push_back(move(world, Point (0, -0.5)));

roles[3].push_back(move(world, Point (0, 0.5)));

roles[4].push_back(move(world, Point (0, 1)));

END_ASSIGN()
END_PLAY()
