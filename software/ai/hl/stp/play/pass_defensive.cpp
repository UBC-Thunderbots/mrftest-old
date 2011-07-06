#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/pass.h"
#include "ai/hl/stp/tactic/offend.h"

using AI::HL::STP::Enemy;
namespace Predicates = AI::HL::STP::Predicates;

/**
 * Pass, defenders have higher priority than passers
 */
BEGIN_PLAY(PassDefensive)
INVARIANT(Predicates::playtype(world, AI::Common::PlayType::PLAY)
	&& Predicates::our_team_size_at_least(world, 4)
	&& Predicates::their_team_size_at_least(world, 1))
APPLICABLE(Predicates::our_ball(world)
	&& (Predicates::ball_midfield(world) || Predicates::ball_in_our_corner(world))
	&& Predicates::ball_on_our_side(world))
DONE(Predicates::baller_can_shoot(world))
FAIL(Predicates::their_ball(world))
BEGIN_ASSIGN()

// STEP 1
goalie_role.push_back(defend_duo_goalie(world));
roles[0].push_back(defend_duo_defender(world));
roles[1].push_back(passer_shoot_dynamic(world));
roles[2].push_back(passee_move_dynamic(world));
roles[3].push_back(block_goal(world, Enemy::closest_friendly_goal(world, 0)));

// STEP 2
roles[1].push_back(passee_receive(world));
roles[2].push_back(block_goal(world, Enemy::closest_friendly_goal(world, 0)));
roles[3].push_back(offend(world));

END_ASSIGN()
END_PLAY()

