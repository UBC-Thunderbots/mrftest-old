#include "ai/hl/stp/tactic/patrol.h"
#include "ai/hl/util.h"
#include <iostream>

using AI::HL::STP::Tactic::Patrol;
using namespace AI::HL::W;

Patrol::Patrol(AI::HL::W::World &world, Point w1, Point w2) : Tactic(world){
	p1 = w1;
	p2 = w2;
}

double Patrol::score(AI::HL::W::Player::Ptr player) const{
	return 1.0 / (1.0 + std::max((player->position() - p1).len(), (player->position() - p2).len()));
}

void Patrol::execute(AI::HL::W::Player::Ptr player){
	if (!player.is()) {
		return;
	}
	
	if ((player->position() - p1).len() < AI::HL::Util::POS_CLOSE) {
		goto_target1 = false;
	} else if ((player->position() - p2).len() < AI::HL::Util::POS_CLOSE) {
		goto_target1 = true;
	}
	if (goto_target1) {
		player->move(p1, (world.ball().position() - player->position()).orientation(), flags, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
	} else {
		player->move(p2, (world.ball().position() - player->position()).orientation(), flags, AI::Flags::MOVE_NORMAL, AI::Flags::PRIO_MEDIUM);
	}
}
