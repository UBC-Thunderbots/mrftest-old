#ifndef AI_STRATEGY_H
#define AI_STRATEGY_H

#include "world/ball.h"
#include "world/field.h"
#include "world/playtype.h"
#include "world/team.h"
#include <libxml++/libxml++.h>

namespace Gtk {
	class Widget;
}
class strategy_factory;

//
// A strategy manages the overall operation of a team. Individual AI
// implementations should extend this class to provide their own strategy.
//
class strategy : public byref, public sigc::trackable {
	public:
		//
		// A pointer to a strategy.
		//
		typedef Glib::RefPtr<strategy> ptr;

		//
		// Runs the strategy for one time tick. It is expected that the strategy
		// will examine the team for which it is responsible, determine if any
		// changes need to be made to the roles or the assignments of robots to
		// roles, make those changes (by means of role::set_robots()), and then
		// call role::tick() for each subsidiary role.
		//
		virtual void tick() = 0;

		//
		// Returns the factory that creates this strategy.
		//
		virtual strategy_factory &get_factory() = 0;

		//
		// Returns the custom UI controls to manage this strategy. A strategy
		// may return a null pointer if it does not wish to display any
		// controls.
		//
		virtual Gtk::Widget *get_ui_controls() = 0;

	protected:
		//
		// Constructs a new strategy. Call this constructor from subclass
		// constructors.
		//
		strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src);

		//
		// Called if a robot has been added to the team. It is expected that the
		// strategy will determine how to allocate the new robot to a role and
		// update that role (by means of role::set_robots()). The strategy may
		// also wish to shuffle other robots between roles or even create new
		// roles based on the changed team size.
		//
		virtual void robot_added(void) = 0;

		//
		// Called if a robot has been removed from our team. It is expected that
		// the strategy will reallocate robots to roles such that the removed
		// robot is no longer assigned to any role.
		//
		virtual void robot_removed(unsigned int index, robot::ptr r) = 0;

		//
		// Sets the current play type. It is expected that the strategy will
		// examine the situation and potentially create new roles and reassign
		// the robots (by means of role::set_robots()).
		//
		virtual void set_playtype(playtype::playtype t) = 0;

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

		//
		// The source of play type information.
		//
		playtype_source &pt_source;
};

//
// A factory for creating strategy objects. An individual AI implementation should
// extend this class to provide an object which can constructs its "strategy"
// objects.
//
class strategy_factory : public noncopyable {
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
		virtual strategy::ptr create_strategy(xmlpp::Element *xml, ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src) = 0;

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

