#include "ai/tactic/patrol.h"
#include "ai/util.h"

#include <iostream>
#include <cstdlib>

namespace {

	class PatrolState : public Player::State {
		public:
			typedef RefPtr<PatrolState> ptr;
			PatrolState(const int& p) : phase(p) {
			}
			int phase;
	};

}

Patrol::Patrol(Player::ptr player, World::ptr world) : Tactic(player), navi(player, world), target_initialized(false) {
}

Patrol::Patrol(Player::ptr player, World::ptr world, const unsigned int& flags, const Point& t1, const Point& t2) : Tactic(player, flags), navi(player, world), target1(t1), target2(t2), target_initialized(true) {
}

void Patrol::tick() {
	navi.set_flags(flags);

	if (!target_initialized) {
		std::cerr << "patrol: no target" << std::endl;
		navi.tick();
		return;
	}

	// This state does not require any validation.
	PatrolState::ptr state(PatrolState::ptr::cast_dynamic(player->get_state(typeid(*this))));
	if (!state) {
		state = PatrolState::ptr(new PatrolState(0));
		player->set_state(typeid(*this), state);
	}

	if ((player->position() - target1).lensq() <= AIUtil::POS_CLOSE) {
		state->phase = 1;
	} else if ((player->position() - target2).lensq() <= AIUtil::POS_CLOSE) {
		state->phase = 0;
	}

	if (!state->phase) {
		navi.set_position(target1);
	} else {
		navi.set_position(target2);
	}

	navi.tick();

}

