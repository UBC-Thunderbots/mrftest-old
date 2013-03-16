#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/tri_attack.h"

using AI::HL::STP::Coordinate;

BEGIN_PLAY(TriOffence)
INVARIANT(playtype(world, PlayType::PLAY) && our_team_size_at_least(world, 6) && !their_ball(world))
//APPLICABLE(true)
APPLICABLE(false)
DONE(false)
FAIL(their_ball(world))
BEGIN_ASSIGN()

// GOALIE
goalie_role.push_back(defend_duo_goalie(world));

// ROLE 1
// duo defender
roles[0].push_back(defend_duo_defender(world));

// ROLE 2
// duo defender
roles[1].push_back(defend_duo_extra1(world));

// ROLE 2
// Tri attack offence formation (active)
roles[2].push_back(tri_attack_primary(world));

// ROLE 4
// Tri attack offence formation #2
roles[3].push_back(tri_attack_secondary(world));

// ROLE 5
// Tri attack offence formation #3
roles[4].push_back(tri_attack_tertiary(world));

END_ASSIGN()
END_PLAY()
