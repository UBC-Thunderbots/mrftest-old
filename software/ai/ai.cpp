#include "ai/ai.h"
#include "ai/setup.h"
#include "util/cacheable.h"
#include "util/dprint.h"
#include "util/object_store.h"
#include <memory>

using AI::AIPackage;
using AI::BE::Backend;

namespace {
	/**
	 * \brief The private per-player state maintained by the AIPackage.
	 */
	struct PrivateState : public ObjectStore::Element {
		/**
		 * \brief A pointer to a PrivateState.
		 */
		typedef std::shared_ptr<PrivateState> Ptr;

		/**
		 * \brief The robot controller driving the player.
		 */
		std::unique_ptr<AI::RC::RobotController> robot_controller;
	};
}

AIPackage::AIPackage(Backend &backend) : backend(backend), high_level(std::unique_ptr<AI::HL::HighLevel>()), navigator(AI::Nav::Navigator::Ptr()), robot_controller_factory(nullptr), show_hl_overlay(true), show_nav_overlay(true), show_rc_overlay(true) {
	backend.signal_tick().connect(sigc::mem_fun(this, &AIPackage::tick));
	backend.signal_draw_overlay().connect(sigc::mem_fun(this, &AIPackage::draw_overlay));
	backend.friendly_team().signal_membership_changed().connect(sigc::mem_fun(this, &AIPackage::attach_robot_controllers));
	robot_controller_factory.signal_changed().connect(sigc::mem_fun(this, &AIPackage::robot_controller_factory_changed));
	high_level.signal_changed().connect(sigc::mem_fun(this, &AIPackage::save_setup));
	high_level.signal_changed().connect(sigc::mem_fun(this, &AIPackage::init_ai_notes));
	navigator.signal_changed().connect(sigc::mem_fun(this, &AIPackage::save_setup));
	robot_controller_factory.signal_changed().connect(sigc::mem_fun(this, &AIPackage::save_setup));
	backend.defending_end().signal_changed().connect(sigc::mem_fun(this, &AIPackage::save_setup));
	backend.friendly_colour().signal_changed().connect(sigc::mem_fun(this, &AIPackage::save_setup));
}

void AIPackage::tick() {
	// Clear all cached data.
	CacheableBase::flush_all();

	// If we have a HighLevel installed, tick it.
	if (high_level.get()) {
		high_level->tick();

		// If we have a Navigator installed, tick it.
		if (navigator.get()) {
			navigator->tick();
		}
	}

	// Tick all the RobotControllers.
	for (std::size_t i = 0; i < backend.friendly_team().size(); ++i) {
		AI::BE::Player::Ptr plr = backend.friendly_team().get(i);
		PrivateState::Ptr state = std::dynamic_pointer_cast<PrivateState>(plr->object_store()[typeid(*this)]);
		if (state->robot_controller) {
			state->robot_controller->tick();
		}
	}
}

void AIPackage::attach_robot_controllers() {
	for (std::size_t i = 0; i < backend.friendly_team().size(); ++i) {
		AI::BE::Player::Ptr plr = backend.friendly_team().get(i);
		PrivateState::Ptr state = std::dynamic_pointer_cast<PrivateState>(plr->object_store()[typeid(*this)]);
		if (!state) {
			state = std::make_shared<PrivateState>();
			if (robot_controller_factory) {
				state->robot_controller = robot_controller_factory->create_controller(AI::RC::W::World(backend), AI::RC::W::Player(plr));
			}
			plr->object_store()[typeid(*this)] = state;
		}
	}
}

void AIPackage::robot_controller_factory_changed() {
	LOG_DEBUG(Glib::ustring::compose("Changing robot controller to %1.", robot_controller_factory ? robot_controller_factory->name() : "<None>"));
	for (std::size_t i = 0; i < backend.friendly_team().size(); ++i) {
		AI::BE::Player::Ptr plr = backend.friendly_team().get(i);
		PrivateState::Ptr state = std::dynamic_pointer_cast<PrivateState>(plr->object_store()[typeid(*this)]);
		state->robot_controller.reset();

		if (robot_controller_factory) {
			state->robot_controller = robot_controller_factory->create_controller(AI::RC::W::World(backend), AI::RC::W::Player(plr));
		}
	}
}

void AIPackage::init_ai_notes() {
	if (high_level.get()) {
		high_level->ai_notes.signal_changed().connect(sigc::mem_fun(this, &AIPackage::ai_notes_changed));
	}
	signal_ai_notes_changed.emit(u8"");
}

void AIPackage::ai_notes_changed() {
	signal_ai_notes_changed.emit(high_level->ai_notes);
}

void AIPackage::draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const {
	if (show_hl_overlay) {
		if (high_level.get()) {
			high_level->draw_overlay(ctx);
		}
	}
	if (show_nav_overlay) {
		if (navigator.get()) {
			navigator->draw_overlay(ctx);
		}
	}
	if (show_rc_overlay) {
		for (std::size_t i = 0; i < backend.friendly_team().size(); ++i) {
			AI::BE::Player::Ptr plr = backend.friendly_team().get(i);
			PrivateState::Ptr state = std::dynamic_pointer_cast<PrivateState>(plr->object_store()[typeid(*this)]);
			if (state->robot_controller) {
				state->robot_controller->draw_overlay(ctx);
			}
		}
	}
}

void AIPackage::save_setup() const {
	AI::Setup setup;
	setup.high_level_name = high_level.get() ? high_level->factory().name() : "";
	setup.navigator_name = navigator.get() ? navigator->factory().name() : "";
	setup.robot_controller_name = robot_controller_factory ? robot_controller_factory->name() : "";
	setup.defending_end = backend.defending_end();
	setup.friendly_colour = backend.friendly_colour();
	setup.save();
}

