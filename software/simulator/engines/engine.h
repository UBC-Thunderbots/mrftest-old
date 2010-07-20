#ifndef SIM_ENGINES_ENGINE_H
#define SIM_ENGINES_ENGINE_H

#include "simulator/ball.h"
#include "simulator/player.h"
#include "util/registerable.h"

namespace Gtk {
	class Widget;
}
class SimulatorEngineFactory;

/**
 * A simulation engine. Individual simulation engines should extend this class
 * to provide actual simulation services.
 */
class SimulatorEngine : public ByRef {
	public:
		/**
		 * Runs a time tick. The engine should update the positions of all its
		 * players and the ball.
		 */
		virtual void tick() = 0;

		/**
		 * Called to retrieve the engine's specific SimulatorBall object. A given engine
		 * must always return the same SimulatorBall object!
		 * \return The SimulatorBall object
		 */
		virtual RefPtr<SimulatorBall> get_ball() = 0;

		/**
		 * Called to create a new SimulatorPlayer. The engine must keep a pointer to the
		 * new object so that SimulatorEngine::tick() can move the SimulatorPlayer.
		 * \return The new SimulatorPlayer object
		 */
		virtual RefPtr<SimulatorPlayer> add_player() = 0;

		/**
		 * Called to remove from the simulation an existing SimulatorPlayer.
		 * \param SimulatorPlayer the SimulatorPlayer to remove
		 */
		virtual void remove_player(RefPtr<SimulatorPlayer> player) = 0;

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
		virtual SimulatorEngineFactory &get_factory() = 0;
};

/**
 * A factory for creating simulator_engines. An individual implementation should
 * extend this class to provide a class which can create objects of a particular
 * derived implementation of SimulatorEngine, and then create a single instance
 * of the factory in a global variable.
 */
class SimulatorEngineFactory : public Registerable<SimulatorEngineFactory> {
	public:
		/**
		 * Constructs a new SimulatorEngine.
		 * \return The new engine
		 */
		virtual RefPtr<SimulatorEngine> create_engine() = 0;

	protected:
		/**
		 * Constructs a SimulatorEngineFactory. This should be invoked at
		 * application startup (by creating a global variable instance of the
		 * implementing class) to register the factory.
		 * \param name a short string naming the factory
		 */
		SimulatorEngineFactory(const Glib::ustring &name) : Registerable<SimulatorEngineFactory>(name) {
		}
};

#endif

