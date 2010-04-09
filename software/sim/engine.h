#ifndef SIMULATOR_ENGINE_H
#define SIMULATOR_ENGINE_H

#include "sim/ball.h"
#include "util/registerable.h"
#include "world/player_impl.h"
#include <libxml++/libxml++.h>

namespace Gtk {
	class Widget;
}
class simulator_engine_factory;

//
// A simulation engine. Individual simulation engines should extend this class
// to provide actual simulation services.
//
class simulator_engine : public byref {
	public:
		//
		// A pointer to a simulator_engine.
		//
		typedef Glib::RefPtr<simulator_engine> ptr;

		//
		// Runs a time tick. Prior to calling this function,
		// player_impl::move_impl() will have been called for all players. It is
		// expected that player_impl::move_impl() will save its parameters and
		// that simulator_engine::tick() will perform the actual updating of
		// player and ball positions.
		//
		virtual void tick() = 0;

		//
		// Called to retrieve the engine's specific ball_impl object. A given
		// engine must always return the same ball_impl object!
		//
		virtual simulator_ball_impl::ptr get_ball() = 0;

		//
		// Called to create a new player. The engine must keep a pointer to the
		// new object so that simulator_engine::tick() can move the player.
		//
		virtual player_impl::ptr add_player() = 0;

		//
		// Called to remove from the simulation an existing player.
		//
		virtual void remove_player(player_impl::ptr player) = 0;

		//
		// Called to retrieve the engine-specific UI controls that will be placed
		// in the simulator window when this engine is activated.
		//
		virtual Gtk::Widget *get_ui_controls() = 0;

		//
		// Called to retrieve the factory object that created the engine.
		//
		virtual simulator_engine_factory &get_factory() = 0;
};

//
// A factory for creating simulator_engines. An individual implementation should
// extend this class to provide a class which can create objects of a particular
// derived implementation of simulator_engine.
//
class simulator_engine_factory : public registerable<simulator_engine_factory> {
	public:
		//
		// Constructs a new simulator_engine.
		//
		virtual simulator_engine::ptr create_engine(xmlpp::Element *xml) = 0;

	protected:
		// 
		// Constructs a simulator_engine_factory.
		//
		simulator_engine_factory(const Glib::ustring &name) : registerable<simulator_engine_factory>(name) {
		}
};

#endif

