#ifndef SIMULATOR_AUTOREF_H
#define SIMULATOR_AUTOREF_H

#include "simulator/ball.h"
#include "world/player_impl.h"
#include <vector>
#include <libxml++/libxml++.h>

namespace Gtk {
	class Widget;
}
class autoref_factory;
class simulator;

//
// An automated referee. Individual autorefs should extend this class to provide
// actual refereeing services.
//
class autoref : public byref {
	public:
		//
		// A pointer to an autoref.
		//
		typedef Glib::RefPtr<autoref> ptr;

		//
		// Runs a time tick. The referee should examine the state of the field
		// and determine if it wishes to take action. Action should be taken by
		// calling ext_drag() on a ball or player, or by calling
		// simulator::set_playtype().
		//
		virtual void tick() = 0;

		//
		// Called to retrieve the autoref-specific UI controls that will be
		// placed in the simulator window when this autoref is activated.
		//
		virtual Gtk::Widget *get_ui_controls() = 0;

		//
		// Called to retrieve the factory object that created the autoref.
		//
		virtual autoref_factory &get_factory() = 0;
};

//
// A factory for creating autorefs. An individual implementation should extend
// this class to provide a class which can create objects of a particular
// derived implementation of autoref.
//
class autoref_factory : public noncopyable {
	public:
		//
		// The type of the map returned by autoref_factory::all().
		//
		typedef std::map<Glib::ustring, autoref_factory *> map_type;

		//
		// The name of the autoref constructed by this factory.
		//
		const Glib::ustring &name() const {
			return the_name;
		}

		//
		// Constructs a new autoref. The individual autoref should hold onto
		// references to the simulator_ball_impl, the two vectors, and the
		// simulator (references, NOT COPIES!), and should examine their values
		// on each time tick to determine if any intervention is necessary.
		//
		virtual autoref::ptr create_autoref(simulator &sim, simulator_ball_impl &the_ball, std::vector<player_impl::ptr> &west_team, std::vector<player_impl::ptr> &east_team, xmlpp::Element *xml) = 0;

		//
		// Gets the collection of all registered autoref factories, keyed by
		// name.
		//
		static const map_type &all();

	protected:
		//
		// Constructs an autoref_factory.
		//
		autoref_factory(const Glib::ustring &name);

		//
		// Destroys an autoref_factory.
		//
		virtual ~autoref_factory();

	private:
		const Glib::ustring the_name;
};

#endif

