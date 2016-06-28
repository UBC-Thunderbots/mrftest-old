/*
 * optimalintercepttest.cpp
 *
 *  Created on: Nov 15, 2015
 *      Author: mathew
 */

#include "../tactic/intercept_v2.h"
#include "ai/hl/stp/tactic/move.h"
#include "ai/hl/stp/tactic/penalty_goalie_new.h"
#include "ai/hl/stp/play/simple_play.h"
#include "ai/hl/stp/coordinate.h"
#include "ai/hl/stp/tactic/ball.h"
#include "ai/hl/stp/tactic/block.h"
#include "ai/hl/stp/enemy.h"

using AI::HL::STP::Coordinate;
using AI::HL::STP::Enemy;
using namespace AI::HL::W;
namespace Predicates = AI::HL::STP::Predicates;


/**
 * Condi
 */
BEGIN_PLAY(Intercept_v2_test)
//INVARIANT((Predicates::playtype(world, AI::Common::PlayType::PREPARE_PENALTY_ENEMY) || Predicates::playtype(world, AI::Common::PlayType::EXECUTE_PENALTY_ENEMY)))
INVARIANT(true)
APPLICABLE(true)
DONE(false)
FAIL(false)
BEGIN_ASSIGN()



// GOALIE
goalie_role.push_back(penalty_goalie_new(world)); //(penalty_goalie_new(world));
//goalie_role.push_back(move(world, Point(-3, 5)));

//roles[0].push_back(move(world, Point(world.ball().position())));
roles[0].push_back(intercept_v2(world));
//roles[0].push_back(move(world, Point(2, 3)));

// ROLE 2
// move to penalty position 2
//roles[1].push_back(move_active(world, Point(2, 3), 2, true, true));
//roles[1].push_back(shoot_goal(world));
//roles[1].push_back(shoot_goal(world, false));
roles[1].push_back(move(world, Point(-1.5, 0)));
//roles[1].push_back(test_shoot(world));

// ROLE 3
// move to penalty position 3
roles[2].push_back(move(world, Point(-3, 5)));

// ROLE 4
// move to penalty position 4
roles[3].push_back(move(world, Point(-3, 5)));

// ROLE 5
// move to penalty position 5
roles[4].push_back(move(world, Point(-3, 5)));


END_ASSIGN()
END_PLAY()

