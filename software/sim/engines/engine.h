#ifndef SIM_ENGINES_ENGINE_H
#define SIM_ENGINES_ENGINE_H

#include "sim/ball.h"
#include "sim/player.h"
#include "util/registerable.h"

namespace Gtk {
	class Widget;
}
class simulator_engine_factory;

/**
 * A simulation engine. Individual simulation engines should extend this class
 * to provide actual simulation services.
 */
class simulator_engine : public byref {
	public:
		/**
		 * A pointer to a simulator_engine.
		 */
		typedef Glib::RefPtr<simulator_engine> ptr;

		/**
		 * Runs a time tick. The engine should update the positions of all its
		 * players and the ball.
		 */
		virtual void tick() = 0;

		/**
		 * Called to retrieve the engine's specific ball object. A given engine
		 * must always return the same ball object!
		 * \return The ball object
		 */
		virtual ball::ptr get_ball() = 0;

		/**
		 * Called to create a new player. The engine must keep a pointer to the
		 * new object so that simulator_engine::tick() can move the player.
		 * \return The new player object
		 */
		virtual player::ptr add_player() = 0;

		/**
		 * Called to remove from the simulation an existing player.
		 * \param player the player to remove
		 */
		virtual void remove_player(player::ptr player) = 0;

		/**
		 * Called to retrieve the engine-specific UI controls that will be placed
		 * in the simulator window when this engine is activated.
		 * \return The UI controls to display, or null to not display controls
		 */
		virtual Gtk::Widget *get_ui_controls() = 0;

		/**
		 * Called to retrieve the factory object that created the engine.
		 * \return The engine factory that created the engine
		 */
		virtual simulator_engine_factory &get_factory() = 0;
};

/**
 * A factory for creating simulator_engines. An individual implementation should
 * extend this class to provide a class which can create objects of a particular
 * derived implementation of simulator_engine, and then create a single instance
 * of the factory in a global variable.
 */
class simulator_engine_factory : public registerable<simulator_engine_factory> {
	public:
		/**
		 * Constructs a new simulator_engine.
		 * \return The new engine
		 */
		virtual simulator_engine::ptr create_engine() = 0;

	protected:
		/**
		 * Constructs a simulator_engine_factory. This should be invoked at
		 * application startup (by creating a global variable instance of the
		 * implementing class) to register the factory.
		 * \param name a short string naming the factory
		 */
		simulator_engine_factory(const Glib::ustring &name) : registerable<simulator_engine_factory>(name) {
		}
};

#endif

