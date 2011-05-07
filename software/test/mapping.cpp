#include "test/mapping.h"
#include "util/string.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <locale>
#include <sstream>

namespace {
	const char * const BUTTON_XML_NAMES[] = {
		"dribble",
		"kick",
		"scram",
	};
	static_assert(G_N_ELEMENTS(BUTTON_XML_NAMES) == JoystickMapping::N_BUTTONS, "BUTTON_XML_NAMES array must be same size as N_BUTTONS enumeration element");

	int to_int(const Glib::ustring &str) {
		std::wistringstream iss(ustring2wstring(str));
		iss.imbue(std::locale("C"));
		int i;
		iss >> i;
		return i;
	}
}

const char * const JoystickMapping::BUTTON_LABELS[] = {
	"Dribble",
	"Kick",
	"Scram",
};
static_assert(G_N_ELEMENTS(JoystickMapping::BUTTON_LABELS) == JoystickMapping::N_BUTTONS, "BUTTON_LABELS array must be same size as N_BUTTONS enumeration element");

JoystickMapping::JoystickMapping(const Glib::ustring &name) : name_(name), name_collate(name_.collate_key()) {
	std::fill(axes, axes + N_AXES, -1);
	std::fill(buttons, buttons + N_BUTTONS, -1);
}

JoystickMapping::JoystickMapping(const xmlpp::Element *elt) : name_(elt->get_attribute_value("name")), name_collate(name_.collate_key()) {
	assert(elt->get_name() == "joystick");
	assert(!name_.empty());
	std::fill(axes, axes + N_AXES, -1);
	std::fill(buttons, buttons + N_BUTTONS, -1);
	const xmlpp::Node::NodeList &group_elts = elt->get_children();
	for (auto i = group_elts.begin(), iend = group_elts.end(); i != iend; ++i) {
		const xmlpp::Element *group_elt = dynamic_cast<const xmlpp::Element *>(*i);
		if (group_elt) {
			int *mappings;
			unsigned int n_mappings;
			const char * const *xml_names;
			if (group_elt->get_name() == "axes") {
				mappings = axes;
				n_mappings = N_AXES;
				xml_names = 0;
			} else if (group_elt->get_name() == "buttons") {
				mappings = buttons;
				n_mappings = N_BUTTONS;
				xml_names = BUTTON_XML_NAMES;
			} else {
				std::abort();
			}
			const xmlpp::Node::NodeList &mapping_elts = group_elt->get_children();
			for (auto j = mapping_elts.begin(), jend = mapping_elts.end(); j != jend; ++j) {
				const xmlpp::Element *mapping_elt = dynamic_cast<const xmlpp::Element *>(*j);
				if (mapping_elt) {
					assert(mapping_elt->get_name() == "mapping");
					const Glib::ustring &logs = mapping_elt->get_attribute_value("logical");
					const Glib::ustring &physs = mapping_elt->get_attribute_value("physical");
					assert(!logs.empty());
					assert(!physs.empty());
					int log;
					if (xml_names) {
						for (log = 0; static_cast<unsigned int>(log) < n_mappings && logs != xml_names[log]; ++log);
					} else {
						log = to_int(logs);
					}
					int phys = to_int(physs);
					assert(0 <= log && static_cast<unsigned int>(log) < n_mappings);
					assert(0 <= phys && phys <= 255);
					mappings[log] = phys;
				}
			}
		}
	}
}

bool JoystickMapping::has_axis(unsigned int axis) const {
	assert(axis < N_AXES);
	return axes[axis] != -1;
}

unsigned int JoystickMapping::axis(unsigned int axis) const {
	assert(has_axis(axis));
	return axes[axis];
}

void JoystickMapping::clear_axis(unsigned int axis) {
	assert(axis < N_AXES);
	axes[axis] = -1;
}

void JoystickMapping::set_axis(unsigned int logical, unsigned int physical) {
	assert(logical < N_AXES);
	assert(physical <= 255);
	axes[logical] = physical;
}

bool JoystickMapping::has_button(unsigned int button) const {
	assert(button < N_BUTTONS);
	return buttons[button] != -1;
}

unsigned int JoystickMapping::button(unsigned int button) const {
	assert(has_button(button));
	return buttons[button];
}

void JoystickMapping::clear_button(unsigned int button) {
	assert(button < N_BUTTONS);
	buttons[button] = -1;
}

void JoystickMapping::set_button(unsigned int logical, unsigned int physical) {
	assert(logical < N_BUTTONS);
	assert(physical <= 255);
	buttons[logical] = physical;
}

void JoystickMapping::save(xmlpp::Element *elt) const {
	assert(elt->get_name() == "joystick");
	elt->set_attribute("name", name_);
	const struct {
		const char *elt_name;
		unsigned int n_mappings;
		const int *mappings;
		const char * const *xml_names;
	} groups[] = {
		{ "axes", N_AXES, axes, 0 },
		{ "buttons", N_BUTTONS, buttons, BUTTON_XML_NAMES },
	};
	for (std::size_t i = 0; i < G_N_ELEMENTS(groups); ++i) {
		xmlpp::Element *group_elt = elt->add_child(groups[i].elt_name);
		for (unsigned int j = 0; j < groups[i].n_mappings; ++j) {
			if (groups[i].mappings[j] >= 0) {
				xmlpp::Element *mapping_elt = group_elt->add_child("mapping");
				mapping_elt->set_attribute("logical", groups[i].xml_names ? groups[i].xml_names[j] : todecu(j));
				mapping_elt->set_attribute("physical", todecu(groups[i].mappings[j]));
			}
		}
	}
}

bool operator<(const JoystickMapping &m1, const JoystickMapping &m2) {
	return m1.name_collate < m2.name_collate;
}

