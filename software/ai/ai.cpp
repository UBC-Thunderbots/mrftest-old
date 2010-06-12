#define DEBUG 0
#include "ai/ai.h"
#include "util/dprint.h"

ai::ai(world::ptr world, clocksource &clk) : the_world(world), clk(clk), the_strategy(), the_rc_factory(0) {
	clk.signal_tick.connect(sigc::mem_fun(this, &ai::tick));
	the_world->friendly.signal_player_added.connect(sigc::mem_fun(this, &ai::player_added));
	the_world->friendly.signal_player_removed.connect(sigc::mem_fun(this, &ai::player_removed));
}

void ai::tick() {
	DPRINT("Tick.");

	// If the field geometry is not yet valid, do nothing.
	if (!the_world->field().valid()) {
		return;
	}

	// Increment the global timestamp.
	the_world->tick_timestamp();

	// First, make the predictors lock in the current time.
	const team * const teams[2] = { &the_world->friendly, &the_world->enemy };
	for (unsigned int i = 0; i < 2; ++i) {
		const team &tm(*teams[i]);
		for (unsigned int j = 0; j < tm.size(); ++j) {
			const robot::ptr bot(tm.get_robot(j));
			bot->lock_time();
			DPRINT(Glib::ustring::compose("Lock time on robot<%1,%2>.", bot->yellow ? 'Y' : 'B', bot->pattern_index));
		}
	}

	// If we have a strategy installed, tick it.
	if (the_strategy) {
		DPRINT("Tick strategy.");
		the_strategy->tick();
	}

	// Tick the robots to drive through robot controllers and XBee.
	DPRINT(Glib::ustring::compose("Friendly team has %1 members.", the_world->friendly.size()));
	for (unsigned int i = 0; i < the_world->friendly.size(); ++i) {
		const player::ptr plr(the_world->friendly.get_player(i));
		DPRINT(Glib::ustring::compose("Tick player<%1,%2>.", plr->yellow ? 'Y' : 'B', plr->pattern_index));
		plr->tick(the_world->playtype() == playtype::halt);
	}
}

void ai::player_added(unsigned int, player::ptr plr) {
	DPRINT(Glib::ustring::compose("player<%1,%2> added.", plr->yellow ? 'Y' : 'B', plr->pattern_index));

	if (the_rc_factory) {
		plr->controller = the_rc_factory->create_controller(plr, plr->yellow, plr->pattern_index);
	}
}

void ai::player_removed(unsigned int, player::ptr plr) {
	DPRINT(Glib::ustring::compose("player<%1,%2> removed.", plr->yellow ? 'Y' : 'B', plr->pattern_index));

	if (plr->controller && plr->controller->refs() != 1) {
		LOG(Glib::ustring::compose("Leak detected of robot_controller for player<%1,%2>.", plr->yellow ? 'Y' : 'B', plr->pattern_index));
	}
	plr->controller.reset();
}

void ai::set_strategy(strategy::ptr strat) {
	if (strat) {
		DPRINT("Install new strategy.");
	} else {
		DPRINT("Remove strategy.");
	}
	the_strategy = strat;
}

void ai::set_robot_controller_factory(robot_controller_factory *fact) {
	if (fact) {
		DPRINT("Install new RC factory.");
	} else {
		DPRINT("Remove RC factory.");
	}

	the_rc_factory = fact;
	for (unsigned int i = 0; i < the_world->friendly.size(); ++i) {
		const player::ptr plr(the_world->friendly.get_player(i));
		DPRINT(Glib::ustring::compose("player<%1,%2> change RC.", plr->yellow ? 'Y' : 'B', plr->pattern_index));
		if (plr->controller && plr->controller->refs() != 1) {
			LOG(Glib::ustring::compose("Leak detected of robot_controller for player<%1,%2>.", plr->yellow ? 'Y' : 'B', plr->pattern_index));
		}
		plr->controller.reset();

		if (the_rc_factory) {
			plr->controller = the_rc_factory->create_controller(plr, plr->yellow, plr->pattern_index);
		}
	}
}

