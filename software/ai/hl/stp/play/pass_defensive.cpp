#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/pass.h"
#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/offend.h"

using AI::HL::STP::Enemy;
namespace Predicates = AI::HL::STP::Predicates;

/**
 * Condition:
 * - ball under team possesion
 * - have at least 4 players (one goalie, one passer, one passee, one defender)
 *
 * Objective:
 * - keep the ball in team possession by passing the ball between the passer and passee
 */
BEGIN_PLAY(PassDefensive)
INVARIANT(Predicates::playtype(world, AI::Common::PlayType::PLAY) && Predicates::our_team_size_at_least(world, 4) && Predicates::their_team_size_at_least(world, 1) && !Predicates::baller_can_shoot(world) && Predicates::baller_under_threat(world))
APPLICABLE(Predicates::our_ball(world) && Predicates::ball_midfield(world))
DONE(Predicates::baller_can_shoot(world) || !Predicates::baller_under_threat(world) || Predicates::none_ball(world))
FAIL(Predicates::their_ball(world) || !Predicates::ball_midfield(world))
BEGIN_ASSIGN()
// GOALIE
goalie_role.push_back(defend_duo_goalie(world));

// ROLE 1
// passer
roles[0].push_back(passer_shoot_dynamic(world));
roles[0].push_back(passee_receive(world));

// ROLE 2
// passee
roles[1].push_back(passee_move_dynamic(world));
roles[1].push_back(defend_duo_extra(world));

// ROLE 3
// defend
roles[2].push_back(defend_duo_defender(world));

// ROLE 4
// offensive support through blocking enemy from the ball
roles[3].push_back(block_goal(world, Enemy::closest_ball(world, 0)));
END_ASSIGN()
END_PLAY()

