#ifndef SIMULATOR_SIMULATOR_H
#define SIMULATOR_SIMULATOR_H

#include "simulator/engine.h"
#include "simulator/team.h"
#include "util/noncopyable.h"
#include "world/ball.h"
#include "world/field.h"
#include <vector>
#include <libxml++/libxml++.h>

//
// The simulator itself.
//
class simulator : public virtual noncopyable {
	public:
		//
		// Constructs a new simulator.
		//
		simulator(xmlpp::Element *xml);

		//
		// Gets the current engine.
		//
		simulator_engine::ptr get_engine() const {
			return engine;
		}

		//
		// Sets the engine to use to implement simulation.
		//
		void set_engine(const Glib::ustring &engine_name);

		//
		// Performs a timestep.
		//
		void update() {
			if (engine)
				engine->update();
		}

		//
		// The field.
		//
		field::ptr fld;

		//
		// The ball, as viewed from the two ends of the field.
		//
		ball::ptr west_ball, east_ball;

		//
		// The two teams.
		//
		simulator_team_data west_team, east_team;

		//
		// The engine.
		//
		simulator_engine::ptr engine;

	private:
		xmlpp::Element *xml;
};

#endif

