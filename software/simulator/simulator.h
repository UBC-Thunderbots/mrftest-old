#ifndef SIMULATOR_SIMULATOR_H
#define SIMULATOR_SIMULATOR_H

#include "simulator/team.h"
#include "util/exact_timer.h"

//
// The simulator itself.
//
class simulator : public noncopyable, public sigc::trackable {
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
		// Sets the current play type.
		//
		void set_playtype(playtype::playtype pt) {
			west_team.set_playtype(pt);
			east_team.set_playtype(pt);
		}

		//
		// The signal emitted after each timestep.
		//
		sigc::signal<void> &signal_updated() {
			return sig_updated;
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

	private:
		//
		// The engine.
		//
		simulator_engine::ptr engine;

		//
		// The configuration element.
		//
		xmlpp::Element *xml;

		//
		// A callback invoked after each timestep.
		//
		sigc::signal<void> sig_updated;

		//
		// The timer.
		//
		exact_timer ticker;

		//
		// Performs a timestep.
		//
		void update() {
/*
if(west_team.get_strategy()){
west_team.get_strategy()->update();
}

if(east_team.get_strategy()){
east_team.get_strategy()->update();
}



			west_team.update();
			east_team.update();
*/
			if (engine)
				engine->update();
			sig_updated.emit();
		}
};

#endif

