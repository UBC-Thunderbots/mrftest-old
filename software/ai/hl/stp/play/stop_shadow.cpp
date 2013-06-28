#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/predicates.h"
#include "ai/hl/stp/tactic/move_stop.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/wait_playtype.h"
#include "ai/hl/stp/tactic/shadow_enemy.h"

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
BEGIN_PLAY(StopShadow)
INVARIANT(Predicates::playtype(world, AI::Common::PlayType::STOP) && Predicates::their_team_size_at_least(world, 2) && Predicates::ball_in_our_corner(world))
APPLICABLE(true)
DONE(false)
FAIL(false)
BEGIN_ASSIGN()
goalie_role.push_back(move(world, Point(world.field().friendly_goal().x + 0.05, 0.0)));

// doesn't matter what the playtype we are waiting for is here, we just need an active tactic
roles[0].push_back(wait_playtype(world, move_stop(world, 0), AI::Common::PlayType::PLAY));

roles[1].push_back(move_stop(world, 1));

roles[2].push_back(shadow_enemy(world, 0));

roles[3].push_back(shadow_enemy(world, 1));

roles[4].push_back(offend(world));

END_ASSIGN()
END_PLAY()

