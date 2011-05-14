#include "ai/hl/stp/play/simple_play.h"

BEGIN_PLAY(JustShoot)
INVARIANT(playtype(world, PlayType::PLAY) && our_team_size_at_least(world, 3) && baller_can_shoot(world))
APPLICABLE(our_ball(world))
DONE(goal(world))
FAIL(their_ball(world))
BEGIN_ASSIGN()
	// GOALIE
	goalie_role.push_back(defend_duo_goalie(world));

	// ROLE 1
	// shoot
	roles[0].push_back(shoot(world));

	// ROLE 2
	// defend
	roles[1].push_back(defend_duo_defender(world));

	// ROLE 3 (optional)
	// offensive support
	roles[2].push_back(offend(world));

	// ROLE 4 (optional)
	// offensive support
	roles[3].push_back(offend(world));
END_ASSIGN()
END_PLAY()

