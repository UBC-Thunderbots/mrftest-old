#include "ai/ai.h"
#include "util/dprint.h"

AI::AI(RefPtr<World> world, ClockSource &clk) : world(world), clk(clk), strategy(), rc_factory(0) {
	clk.signal_tick.connect(sigc::mem_fun(this, &AI::tick));
	world->friendly.signal_player_added.connect(sigc::mem_fun(this, &AI::player_added));
	world->friendly.signal_player_removed.connect(sigc::mem_fun(this, &AI::player_removed));
}

void AI::tick() {
	// If the field geometry is not yet valid, do nothing.
	if (!world->field().valid()) {
		return;
	}

	// Increment the global timestamp.
	world->tick_timestamp();

	// First, make the predictors lock in the current time.
	const Team * const teams[2] = { &world->friendly, &world->enemy };
	for (unsigned int i = 0; i < 2; ++i) {
		const Team &tm(*teams[i]);
		for (unsigned int j = 0; j < tm.size(); ++j) {
			const RefPtr<Robot> bot(tm.get_robot(j));
			bot->lock_time();
		}
	}

	// If we have a Strategy installed, tick it.
	if (strategy) {
		if (overlay) {
			overlay->save();
			overlay->set_operator(Cairo::OPERATOR_CLEAR);
			overlay->paint();
			overlay->set_operator(Cairo::OPERATOR_OVER);
		}
		strategy->tick(overlay);
		if (overlay) {
			overlay->restore();
		}
	}

	// Tick the robots to drive through robot controllers and XBee.
	for (unsigned int i = 0; i < world->friendly.size(); ++i) {
		const RefPtr<Player> plr(world->friendly.get_player(i));
		plr->tick(world->playtype() == PlayType::HALT || !strategy);
	}
}

void AI::player_added(unsigned int, RefPtr<Player> plr) {
	if (rc_factory) {
		plr->controller = rc_factory->create_controller(plr, plr->yellow, plr->pattern_index);
	}
}

void AI::player_removed(unsigned int, RefPtr<Player> plr) {
	if (plr->controller && plr->controller->refs() != 1) {
		LOG_WARN(Glib::ustring::compose("Leak detected of robot_controller for player<%1,%2>.", plr->yellow ? 'Y' : 'B', plr->pattern_index));
	}
	plr->controller.reset();
}

void AI::set_strategy(RefPtr<Strategy2> strat) {
	if (strategy != strat) {
		LOG_DEBUG(Glib::ustring::compose("Changing strategy to %1.", strat ? strat->get_factory().name : "<None>"));
		strategy = strat;
	}
}

void AI::set_robot_controller_factory(RobotControllerFactory *fact) {
	if (rc_factory != fact) {
		LOG_DEBUG(Glib::ustring::compose("Changing robot controller to %1.", fact ? fact->name : "<None>"));
		rc_factory = fact;
		for (unsigned int i = 0; i < world->friendly.size(); ++i) {
			const RefPtr<Player> plr(world->friendly.get_player(i));
			if (plr->controller && plr->controller->refs() != 1) {
				LOG_WARN(Glib::ustring::compose("Leak detected of robot_controller for player<%1,%2>.", plr->yellow ? 'Y' : 'B', plr->pattern_index));
			}
			plr->controller.reset();

			if (rc_factory) {
				plr->controller = rc_factory->create_controller(plr, plr->yellow, plr->pattern_index);
			}
		}
	}
}

void AI::set_overlay(Cairo::RefPtr<Cairo::Context> ovl) {
	overlay = ovl;
}

