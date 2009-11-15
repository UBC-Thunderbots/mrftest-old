#include "ai/strategy.h"
#include <stdexcept>

namespace {
	strategy_factory::map_type &get_map() {
		static strategy_factory::map_type objects;
		return objects;
	}
}

strategy::strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team, playtype_source &pt_src) : the_ball(ball), the_field(field), the_team(team), pt_source(pt_src) {
	team->signal_robot_added().connect(sigc::mem_fun(*this, &strategy::robot_added));
	team->signal_robot_removed().connect(sigc::mem_fun(*this, &strategy::robot_removed));
	pt_source.signal_playtype_changed().connect(sigc::mem_fun(*this, &strategy::set_playtype));
}

strategy_factory::strategy_factory(const Glib::ustring &name) : the_name(name) {
	if (get_map().count(name))
		throw std::logic_error(Glib::locale_from_utf8(Glib::ustring::compose("Duplicate strategy name \"%1\"", name)));
	get_map()[name] = this;
}

strategy_factory::~strategy_factory() {
	get_map().erase(name());
}

const strategy_factory::map_type &strategy_factory::all() {
	return get_map();
}

