#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/tri_attack.h"
using AI::HL::STP::Coordinate;

BEGIN_PLAY(TriOffence)
INVARIANT(playtype(world, PlayType::PLAY) && our_team_size_at_least(world, 5) && !their_ball(world))
//APPLICABLE(true)
APPLICABLE(false)
DONE(false)
FAIL(their_ball(world))
BEGIN_ASSIGN()

// GOALIE
goalie_role.push_back(defend_duo_goalie(world));

// ROLE 1
// Tri attack offence formation (active)
//roles[0].push_back(tri_attack_primary(world));

// ROLE 2 
// Tri attack offence formation #2
//roles[1].push_back(tri_attack_secondaries(world));

// ROLE 3 
// dTri attack offence formation #3
//roles[2].push_back(tri_attack_tertiary(world));

// ROLE 4
// duo defender
//roles[3].push_back(defend_duo_defender(world));

// ROLE 5 (optional)
// duo defender
roles[4].push_back(defend_duo_extra1(world));

END_ASSIGN()
END_PLAY()

