#include "ai/ai.h"
#include "util/cacheable.h"
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

AIPackage::AIPackage(Backend &backend) : backend(backend), high_level(AI::HL::HighLevel::Ptr()), navigator(AI::Nav::Navigator::Ptr()), robot_controller_factory(0) {
	backend.signal_tick().connect(sigc::mem_fun(this, &AIPackage::tick));
	backend.signal_draw_overlay().connect(sigc::mem_fun(this, &AIPackage::draw_overlay));
	backend.friendly_team().signal_robot_added().connect(sigc::mem_fun(this, &AIPackage::player_added));
	robot_controller_factory.signal_changed().connect(sigc::mem_fun(this, &AIPackage::robot_controller_factory_changed));
}

void AIPackage::tick() {
	// Clear all cached data.
	CacheableBase::flush_all();

	// If we have a HighLevel installed, tick it.
	AI::HL::HighLevel::Ptr hl = high_level;
	if (hl.is()) {
		hl->tick();

		// If we have a Navigator installed, tick it.
		AI::Nav::Navigator::Ptr nav = navigator;
		if (nav.is()) {
			nav->tick();
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
		state->robot_controller = robot_controller_factory->create_controller(backend, plr);
	}
	plr->object_store()[typeid(*this)] = state;
}

void AIPackage::robot_controller_factory_changed() {
	LOG_DEBUG(Glib::ustring::compose("Changing robot controller to %1.", robot_controller_factory ? robot_controller_factory->name() : "<None>"));
	for (std::size_t i = 0; i < backend.friendly_team().size(); ++i) {
		AI::BE::Player::Ptr plr = backend.friendly_team().get(i);
		PrivateState::Ptr state = PrivateState::Ptr::cast_dynamic(plr->object_store()[typeid(*this)]);
		if (state->robot_controller.is() && state->robot_controller->refs() != 1) {
			LOG_WARN("Leak detected of robot_controller.");
		}
		state->robot_controller.reset();

		if (robot_controller_factory) {
			state->robot_controller = robot_controller_factory->create_controller(backend, plr);
		}
	}
}

void AIPackage::draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const {
	AI::HL::HighLevel::Ptr hl = high_level;
	if (hl.is()) {
		hl->draw_overlay(ctx);
	}
}

