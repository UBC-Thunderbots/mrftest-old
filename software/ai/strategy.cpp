#include "ai/strategy.h"

namespace {
	strategy_factory::map_type &get_map() {
		static strategy_factory::map_type objects;
		return objects;
	}
}

strategy::strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team) : the_ball(ball), the_field(field), the_team(team) {
	team->signal_robot_added().connect(sigc::mem_fun(*this, &strategy::robot_added));
	team->signal_robot_removed().connect(sigc::mem_fun(*this, &strategy::robot_removed));
}

strategy_factory::strategy_factory(const Glib::ustring &name) : the_name(name) {
	get_map()[name] = this;
}

strategy_factory::~strategy_factory() {
	get_map().erase(name());
}

const strategy_factory::map_type &strategy_factory::all() {
	return get_map();
}

