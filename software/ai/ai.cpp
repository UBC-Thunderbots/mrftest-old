#include "ai/ai.h"
#include "util/dprint.h"
#include "util/objectstore.h"

using AI::AIPackage;
using AI::BE::Backend;

namespace {
	/**
	 * The private per-player state maintained by the AIPackage.
	 */
	struct PrivateState : public ObjectStore::Element {
		/**
		 * A pointer to a PrivateState.
		 */
		typedef RefPtr<PrivateState> Ptr;

		/**
		 * The robot controller driving the player.
		 */
		AI::RC::RobotController::Ptr robot_controller;
	};
}

AIPackage::AIPackage(Backend &backend) : backend(backend), coach(AI::Coach::Coach::Ptr(0)), navigator(AI::Nav::Navigator::Ptr(0)), robot_controller_factory(0) {
	backend.signal_tick().connect(sigc::mem_fun(this, &AIPackage::tick));
	backend.friendly_team().signal_robot_added().connect(sigc::mem_fun(this, &AIPackage::player_added));
	backend.friendly_team().signal_robot_removing().connect(sigc::mem_fun(this, &AIPackage::player_removing));
	robot_controller_factory.signal_changed().connect(sigc::mem_fun(this, &AIPackage::robot_controller_factory_changed));
}

AIPackage::~AIPackage() {
}

void AIPackage::tick() {
	// If we have a Coach installed, tick it.
	AI::Coach::Coach::Ptr c = coach;
	if (c.is()) {
		c->tick();
		// If we have a Strategy installed, tick it.
		AI::HL::Strategy::Ptr strategy = backend.strategy();
		if (strategy.is()) {
			strategy->tick();
			// If the strategy resigned, try to find another one and tick it instead.
			if (strategy->has_resigned()) {
				c->tick();
				strategy = backend.strategy();
				if (strategy.is()) {
					strategy->tick();
				}
			}
			// If we have a Navigator installed, tick it.
			AI::Nav::Navigator::Ptr nav = navigator;
			if (nav.is()) {
				nav->tick();
			}
		}
	}

	// Tick all the RobotControllers.
	for (std::size_t i = 0; i < backend.friendly_team().size(); ++i) {
		AI::BE::Player::Ptr plr = backend.friendly_team().get(i);
		PrivateState::Ptr state = PrivateState::Ptr::cast_dynamic(plr->object_store()[typeid(*this)]);
		AI::RC::RobotController::Ptr rc = state->robot_controller;
		if (rc.is()) {
			rc->tick();
		}
	}
}

void AIPackage::player_added(std::size_t idx) {
	AI::BE::Player::Ptr plr = backend.friendly_team().get(idx);
	PrivateState::Ptr state(new PrivateState);
	if (robot_controller_factory) {
		state->robot_controller = robot_controller_factory->create_controller(plr);
	}
	plr->object_store()[typeid(*this)] = state;
}

void AIPackage::player_removing(std::size_t idx) {
	// Even though normally an ObjectStore clears its contents automatically when destroyed,
	// here, we need to clear our state block explicitly,
	// because otherwise there would be a circular reference from Player to PrivateState to RobotController to Player.
	AI::BE::Player::Ptr plr = backend.friendly_team().get(idx);
	PrivateState::Ptr state = PrivateState::Ptr::cast_dynamic(plr->object_store()[typeid(*this)]);
	plr->object_store()[typeid(*this)].reset();
	if (state->robot_controller.is() && state->robot_controller->refs() != 1) {
		LOG_WARN("Leak detected of robot_controller.");
	}
}

void AIPackage::robot_controller_factory_changed() {
	LOG_DEBUG(Glib::ustring::compose("Changing robot controller to %1.", robot_controller_factory ? robot_controller_factory->name : "<None>"));
	for (unsigned int i = 0; i < backend.friendly_team().size(); ++i) {
		AI::BE::Player::Ptr plr = backend.friendly_team().get(i);
		PrivateState::Ptr state = PrivateState::Ptr::cast_dynamic(plr->object_store()[typeid(*this)]);
		if (state->robot_controller.is() && state->robot_controller->refs() != 1) {
			LOG_WARN("Leak detected of robot_controller.");
		}
		state->robot_controller.reset();

		if (robot_controller_factory) {
			state->robot_controller = robot_controller_factory->create_controller(plr);
		}
	}
}

