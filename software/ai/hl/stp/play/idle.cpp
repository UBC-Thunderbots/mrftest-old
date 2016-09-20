#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/idle.h"
#include "ai/hl/stp/tactic/move.h"

BEGIN_DEC(Idle)
INVARIANT(true)
APPLICABLE(true)
END_DEC(Idle)

BEGIN_DEF(Idle)
DONE(false)
FAIL(false)
EXECUTE()
for (unsigned int i = 0; i < TEAM_MAX_SIZE; i++) {
	tactics[i] = Tactic::move(world, Coordinate(world, Point::of_angle(Angle::of_degrees(360.0 * i / TEAM_MAX_SIZE)),
				Coordinate::YType::ABSOLUTE, Coordinate::OriginType::BALL));
}

for (unsigned int i = 0; i < TEAM_MAX_SIZE; i++) {
	wait(caller, tactics[i].get());
}
END_DEF(Idle)
