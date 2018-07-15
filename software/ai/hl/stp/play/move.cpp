//
// Created by evan on 19/05/18.
//

#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/tactic/defend.h"
#include "ai/hl/stp/tactic/idle.h"
#include <random>



BEGIN_DEC(MOVE)
INVARIANT(true)
APPLICABLE(true)
END_DEC(MOVE)

BEGIN_DEF(MOVE)
DONE(false)
FAIL(false)
EXECUTE()
Point test = Point(world.field().length() / 2, 0);
tactics[0] = Tactic::goalie(world);
tactics[1] = Tactic::move(world, world.field().friendly_crease_neg_corner());
tactics[2] = Tactic::move(world, world.field().friendly_crease_neg_endline());
tactics[3] = Tactic::move(world, world.field().friendly_crease_pos_corner());
tactics[4] = Tactic::move(world, world.field().friendly_crease_pos_endline());
// tactics[4] = Tactic::move(world);

END_DEF(MOVE)