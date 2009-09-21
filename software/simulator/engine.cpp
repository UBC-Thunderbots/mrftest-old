#include "simulator/engine.h"

namespace {
	std::map<Glib::ustring, simulator_engine_factory *> &get_map() {
		static std::map<Glib::ustring, simulator_engine_factory *> objects;
		return objects;
	}
}

const std::map<Glib::ustring, simulator_engine_factory *> &simulator_engine_factory::all() {
	return get_map();
}

simulator_engine_factory::simulator_engine_factory(const Glib::ustring &name) : the_name(name) {
	get_map()[name] = this;
}

simulator_engine_factory::~simulator_engine_factory() {
	get_map().erase(name());
}

