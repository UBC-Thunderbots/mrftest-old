#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/tactic/pass.h"
#include "ai/hl/stp/play/simple_play.h"

using AI::HL::STP::Enemy;
namespace Predicates = AI::HL::STP::Predicates;

/**
 * Pass, with more offensive tactics
 */
BEGIN_PLAY(PassOffensive)
INVARIANT(false && Predicates::playtype(world, AI::Common::PlayType::PLAY)
		&& Predicates::our_team_size_at_least(world, 3)
		&& Predicates::their_team_size_at_least(world, 1))
APPLICABLE(Predicates::our_ball(world)
		&& (Predicates::ball_midfield(world) || Predicates::ball_in_their_corner(world))
		&& Predicates::ball_on_their_side(world))
DONE(Predicates::baller_can_shoot(world))
FAIL(Predicates::their_ball(world))
BEGIN_ASSIGN()

// STEP 1
goalie_role.push_back(goalie_dynamic(world, 1));
roles[0].push_back(passer_shoot_dynamic(world));
roles[1].push_back(passee_move_dynamic(world));
roles[2].push_back(defend_duo_defender(world));
roles[3].push_back(block_ball(world, Enemy::closest_ball(world, 0)));

// STEP 2
goalie_role.push_back(goalie_dynamic(world, 1));
roles[0].push_back(passee_receive(world));
roles[1].push_back(offend(world));
roles[2].push_back(defend_duo_defender(world));
roles[3].push_back(offend_secondary(world));

END_ASSIGN()
END_PLAY()

