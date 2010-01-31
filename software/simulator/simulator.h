#ifndef SIMULATOR_SIMULATOR_H
#define SIMULATOR_SIMULATOR_H

#include "log/writer/writer.h"
#include "simulator/autoref.h"
#include "simulator/team.h"
#include "util/clocksource.h"

//
// The simulator itself.
//
// WARNING - WARNING - WARNING
// A lot of work is done in the constructors of this object and its contents.
// DO NOT CHANGE THE ORDER OF THE VARIABLES IN THIS CLASS.
// You may cause a segfault if you do.
//
class simulator : public noncopyable, public sigc::trackable {
	public:
		//
		// Constructs a new simulator.
		//
		simulator(xmlpp::Element *xml, clocksource &clk);

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
		// Gets the current autoref.
		//
		autoref::ptr get_autoref() const {
			return ref;
		}

		//
		// Sets the autoref to use.
		//
		void set_autoref(const Glib::ustring &autoref_name);

		//
		// Gets the current play type.
		//
		playtype::playtype current_playtype() const {
			return cur_playtype;
		}

		//
		// Returns a signal fired when the play type changes.
		//
		sigc::signal<void, playtype::playtype> &signal_playtype_changed() {
			return sig_playtype_changed;
		}

		//
		// Sets the current play type.
		//
		void set_playtype(playtype::playtype pt) {
			cur_playtype = pt;
			sig_playtype_changed.emit(pt);
		}

		//
		// Starts logging.
		//
		void start_logging() {
			logger = log_writer::create(clksrc, fld, west_ball, west_team.west_view, east_team.east_view);
		}

		//
		// Stops logging.
		//
		void stop_logging() {
			logger.reset();
		}

	private:
		//
		// The current play type.
		//
		playtype::playtype cur_playtype;

		//
		// Emitted when the play type changes.
		//
		sigc::signal<void, playtype::playtype> sig_playtype_changed;

		//
		// The current ball_impl.
		//
		simulator_ball_impl::ptr the_ball_impl;

	public:
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
		// The autoref.
		//
		autoref::ptr ref;

		//
		// The configuration element.
		//
		xmlpp::Element *xml;

		//
		// The clock source.
		//
		clocksource &clksrc;

		//
		// The log writer.
		//
		log_writer::ptr logger;

		//
		// Handles a timer tick.
		//
		void tick();
};

#endif

