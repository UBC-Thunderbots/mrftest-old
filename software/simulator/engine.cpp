#include "simulator/engine.h"

namespace {
	simulator_engine_factory::map_type &get_map() {
		static simulator_engine_factory::map_type objects;
		return objects;
	}
}

const simulator_engine_factory::map_type &simulator_engine_factory::all() {
	return get_map();
}

simulator_engine_factory::simulator_engine_factory(const Glib::ustring &name) : the_name(name) {
	get_map()[name] = this;
}

simulator_engine_factory::~simulator_engine_factory() {
	get_map().erase(name());
}

