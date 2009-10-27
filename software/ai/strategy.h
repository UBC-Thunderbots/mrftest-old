#ifndef AI_STRATEGY_H
#define AI_STRATEGY_H

#include "util/byref.h"
#include "util/noncopyable.h"
#include "world/ball.h"
#include "world/field.h"
#include "world/playtype.h"
#include "world/team.h"
#include "ai/role.h"
#include <map>
#include <glibmm.h>
#include <gtkmm.h>
#include <libxml++/libxml++.h>
#include <sigc++/sigc++.h>

class strategy_factory;

//
// A strategy manages the overall operation of a team. Individual AI implementations
// should extend this class to provide their own strategy.
//
class strategy : public virtual byref, public virtual sigc::trackable {
	public:
		//
		// A pointer to a strategy.
		//
		typedef Glib::RefPtr<strategy> ptr;

		//
		// Runs the AI for one time tick.
		//
		virtual void update() = 0;

		//
		// Sets the current play type.
		//
		virtual void set_playtype(playtype::playtype t) = 0;

		//
		// Returns the factory that creates this strategy.
		//
		virtual strategy_factory &get_factory() = 0;

		//
		// Returns the custom UI controls to manage this strategy.
		//
		virtual Gtk::Widget *get_ui_controls() = 0;

	protected:
		//
		// Constructs a new strategy. Call this constructor from subclass constructors.
		//
		strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team);

    //
    // function dedicated to handle if a robot has been added to our team
    //
    virtual void handleRobotAdded(void) = 0;

    //
    // function dedicated to handle if a robot has been removed from our team
    //
    virtual void handleRobotRemoved(unsigned int index, robot::ptr r) = 0;

		//
		// The ball.
		//
		const ball::ptr the_ball;

		//
		// The field.
		//
		const field::ptr the_field;

		//
		// The team this strategy controls.
		//
		const controlled_team::ptr the_team;
};

//
// A factory for creating strategy objects. An individual AI implementation should
// extend this class to provide an object which can constructs its "strategy"
// objects.
//
class strategy_factory : public virtual noncopyable {
	public:
		//
		// The type of the map returned by strategy_factory::all().
		//
		typedef std::map<Glib::ustring, strategy_factory *> map_type;

		//
		// The name of the strategy constructed by this factory.
		//
		const Glib::ustring &name() const {
			return the_name;
		}

		//
		// Constructs a new strategy.
		//
		virtual strategy::ptr create_strategy(xmlpp::Element *xml, ball::ptr ball, field::ptr field, controlled_team::ptr team) = 0;

		//
		// Gets a collection of all registered strategy factories, keyed by name.
		//
		static const map_type &all();

	protected:
		//
		// Constructs a strategy_factory.
		//
		strategy_factory(const Glib::ustring &name);

		//
		// Destroys a strategy_factory.
		//
		virtual ~strategy_factory();

	private:
		const Glib::ustring the_name;
};

#endif

