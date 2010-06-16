#include "ai/tactic/patrol.h"
#include "ai/util.h"

#include <iostream>
#include <cstdlib>

namespace {

	class patrol_state : public player::state {
		public:
			typedef Glib::RefPtr<patrol_state> ptr;
			patrol_state(const int& p) : phase(p) {
			}
			int phase;
	};

}

patrol::patrol(player::ptr player, world::ptr world) : tactic(player), navi(player, world), target_initialized(false) {
}

patrol::patrol(player::ptr player, world::ptr world, const unsigned int& flags, const point& t1, const point& t2) : tactic(player, flags), navi(player, world), target1(t1), target2(t2), target_initialized(true) {
}

void patrol::tick() {
	navi.set_flags(flags);

	if (!target_initialized) {
		std::cerr << "patrol: no target" << std::endl;
		navi.tick();
		return;
	}

	// This state does not require any validation.
	patrol_state::ptr state(patrol_state::ptr::cast_dynamic(the_player->get_state(typeid(*this))));
	if (!state) {
		state = patrol_state::ptr(new patrol_state(0));
		the_player->set_state(typeid(*this), state);
	}

	if ((the_player->position() - target1).lensq() <= ai_util::POS_CLOSE) {
		state->phase = 1;
	} else if ((the_player->position() - target2).lensq() <= ai_util::POS_CLOSE) {
		state->phase = 0;
	}

	if (!state->phase) {
		navi.set_position(target1);
	} else {
		navi.set_position(target2);
	}

	navi.tick();

}

