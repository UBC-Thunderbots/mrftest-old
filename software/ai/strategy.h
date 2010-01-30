#ifndef AI_STRATEGY_H
#define AI_STRATEGY_H

#include "util/registerable.h"
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
		strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team);

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
		virtual void robot_removed(unsigned int index, player::ptr r) = 0;

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
};

//
// A factory for creating strategy objects. An individual AI implementation should
// extend this class to provide an object which can constructs its "strategy"
// objects.
//
class strategy_factory : public registerable<strategy_factory> {
	public:
		//
		// Constructs a new strategy.
		//
		virtual strategy::ptr create_strategy(xmlpp::Element *xml, ball::ptr ball, field::ptr field, controlled_team::ptr team) = 0;

	protected:
		//
		// Constructs a strategy_factory.
		//
		strategy_factory(const Glib::ustring &name) : registerable<strategy_factory>(name) {
		}
};

#endif

