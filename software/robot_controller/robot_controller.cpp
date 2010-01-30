#include "robot_controller/robot_controller.h"
#include "util/locale.h"
#include <stdexcept>

namespace {
	static robot_controller_factory::map_type &get_map() {
		static robot_controller_factory::map_type objects;
		return objects;
	}
}

const robot_controller_factory::map_type &robot_controller_factory::all() {
	return get_map();
}

robot_controller_factory::robot_controller_factory(const Glib::ustring &name) : the_name(name) {
	initialize_locale();
	if (get_map().count(name))
		throw std::logic_error(Glib::locale_from_utf8(Glib::ustring::compose("Duplicate robot controller name \"%1\"", name)));
	get_map()[name] = this;
}

robot_controller_factory::~robot_controller_factory() {
	get_map().erase(name());
}

