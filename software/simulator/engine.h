#ifndef SIMULATOR_ENGINE_H
#define SIMULATOR_ENGINE_H

#include "util/byref.h"
#include "world/ball_impl.h"
#include "world/player_impl.h"
#include <map>
#include <glibmm.h>

//
// A simulation engine. Individual simulation engines should extend this class
// to provide actual simulation services.
//
class simulator_engine : public virtual byref {
	public:
		//
		// A pointer to a simulator_engine.
		//
		typedef Glib::RefPtr<simulator_engine> ptr;

		//
		// Called once per AI frame to perform an update of the world state.
		//
		virtual void update() = 0;

		//
		// Called to retrieve the engine's specific ball_impl object.
		//
		virtual ball_impl::ptr get_ball() = 0;

		//
		// Called to create a new player. The engine must keep a pointer to the
		// new object and update its state.
		//
		virtual player_impl::ptr add_player() = 0;

		//
		// Called to remove from the simulation an existing player.
		//
		virtual void remove_player(player_impl::ptr player) = 0;
};

//
// A factory for creating simulator_engines. An individual implementation should
// extend this class to provide a class which can create objects of a particular
// derived implementation of simulator_engine.
//
class simulator_engine_factory : public virtual noncopyable {
	public:
		//
		// The name of the simulation engine constructed by this factory.
		//
		const Glib::ustring &name() const {
			return the_name;
		}

		//
		// Constructs a new simulator_engine.
		//
		virtual simulator_engine::ptr create_engine() = 0;

		//
		// Gets the collection of all registered engine factories, keyed by name.
		//
		static const std::map<Glib::ustring, simulator_engine_factory *> &all();

	protected:
		//
		// Constructs a simulator_engine_factory.
		//
		simulator_engine_factory(const Glib::ustring &name);

		//
		// Destroys a simulator_engine_factory.
		//
		virtual ~simulator_engine_factory();

	private:
		const Glib::ustring the_name;
};

#endif

