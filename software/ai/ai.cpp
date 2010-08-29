#include "ai/ai.h"
#include "util/dprint.h"

AI::AI::AI(World &world, ClockSource &clk) : world(world), clk(clk), coach(), rc_factory(0) {
	clk.signal_tick.connect(sigc::mem_fun(this, &AI::tick));
	world.friendly.signal_player_added.connect(sigc::mem_fun(this, &AI::player_added));
	world.friendly.signal_player_removed.connect(sigc::mem_fun(this, &AI::player_removed));
}

AI::Coach::Ptr AI::AI::get_coach() const {
	return coach;
}

void AI::AI::set_coach(Coach::Ptr c) {
	if (coach != c) {
		LOG_DEBUG(Glib::ustring::compose("Changing coach to %1.", c.is() ? c->get_factory().name : "<None>"));
		coach = c;
	}
}

void AI::AI::tick() {
	// If the field geometry is not yet valid, do nothing.
	if (!world.field().valid()) {
		return;
	}

	// Increment the global timestamp.
	world.tick_timestamp();

	// First, make the predictors lock in the current time.
	const Team * const teams[2] = { &world.friendly, &world.enemy };
	for (unsigned int i = 0; i < 2; ++i) {
		const Team &tm(*teams[i]);
		for (unsigned int j = 0; j < tm.size(); ++j) {
			const Robot::Ptr bot(tm.get_robot(j));
			bot->lock_time();
		}
	}

	// If we have a Coach installed, tick it. It will drive the rest of the AI.
	if (coach.is()) {
		coach->tick();
	}

	// Tick the robots to drive through robot controllers and XBee.
	for (unsigned int i = 0; i < world.friendly.size(); ++i) {
		const Player::Ptr plr(world.friendly.get_player(i));
		plr->tick(world.playtype() == PlayType::HALT || !coach.is() || !coach->get_strategy().is());
	}
}

void AI::AI::player_added(unsigned int, Player::Ptr plr) {
	if (rc_factory) {
		plr->controller = rc_factory->create_controller(plr, plr->yellow, plr->pattern_index);
	}
}

void AI::AI::player_removed(unsigned int, Player::Ptr plr) {
	if (plr->controller.is() && plr->controller->refs() != 1) {
		LOG_WARN(Glib::ustring::compose("Leak detected of robot_controller for player<%1,%2>.", plr->yellow ? 'Y' : 'B', plr->pattern_index));
	}
	plr->controller.reset();
}

void AI::AI::set_robot_controller_factory(RobotController::RobotControllerFactory *fact) {
	if (rc_factory != fact) {
		LOG_DEBUG(Glib::ustring::compose("Changing robot controller to %1.", fact ? fact->name : "<None>"));
		rc_factory = fact;
		for (unsigned int i = 0; i < world.friendly.size(); ++i) {
			const Player::Ptr plr(world.friendly.get_player(i));
			if (plr->controller.is() && plr->controller->refs() != 1) {
				LOG_WARN(Glib::ustring::compose("Leak detected of robot_controller for player<%1,%2>.", plr->yellow ? 'Y' : 'B', plr->pattern_index));
			}
			plr->controller.reset();

			if (rc_factory) {
				plr->controller = rc_factory->create_controller(plr, plr->yellow, plr->pattern_index);
			}
		}
	}
}

