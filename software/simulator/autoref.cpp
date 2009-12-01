#include "simulator/autoref.h"
#include <stdexcept>

namespace {
	autoref_factory::map_type &get_map() {
		static autoref_factory::map_type objects;
		return objects;
	}
}

const autoref_factory::map_type &autoref_factory::all() {
	return get_map();
}

autoref_factory::autoref_factory(const Glib::ustring &name) : the_name(name) {
	if (get_map().count(name))
		throw std::logic_error(Glib::locale_from_utf8(Glib::ustring::compose("Duplicate autoref name \"%1\"", name)));
	get_map()[name] = this;
}

autoref_factory::~autoref_factory() {
	get_map().erase(name());
}

