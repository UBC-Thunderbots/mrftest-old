#include "test/common/mapping.h"
#include "util/string.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <locale>
#include <sstream>
#include <string>

namespace {
	const char *const BUTTON_XML_NAMES[] = {
		u8"dribble",
		u8"kick",
		u8"scram",
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

const char *const JoystickMapping::BUTTON_LABELS[] = {
	u8"Dribble",
	u8"Kick",
	u8"Scram",
};
static_assert(G_N_ELEMENTS(JoystickMapping::BUTTON_LABELS) == JoystickMapping::N_BUTTONS, u8"BUTTON_LABELS array must be same size as N_BUTTONS enumeration element");

constexpr unsigned int JoystickMapping::N_AXES;

JoystickMapping::JoystickMapping(const Joystick::Identifier &identifier) : identifier_(identifier), name_collate(identifier_.name.collate_key()) {
	std::fill(axes, axes + N_AXES, -1);
	std::fill(buttons, buttons + N_BUTTONS, -1);
}

JoystickMapping::JoystickMapping(const xmlpp::Element *elt) :
		identifier_{
			elt->get_attribute_value(u8"name"),
			static_cast<uint16_t>(std::stoul(elt->get_attribute_value(u8"bus_type"))),
			static_cast<uint16_t>(std::stoul(elt->get_attribute_value(u8"vendor_id"))),
			static_cast<uint16_t>(std::stoul(elt->get_attribute_value(u8"product_id"))),
			static_cast<uint16_t>(std::stoul(elt->get_attribute_value(u8"version"))),
		},
		name_collate(identifier_.name.collate_key()) {
	assert(elt->get_name() == u8"joystick");
	assert(!identifier_.name.empty());
	std::fill(axes, axes + N_AXES, -1);
	std::fill(buttons, buttons + N_BUTTONS, -1);
	const xmlpp::Node::NodeList &group_elts = elt->get_children();
	for (const xmlpp::Node *group_elt_node : group_elts) {
		const xmlpp::Element *group_elt = dynamic_cast<const xmlpp::Element *>(group_elt_node);
		if (group_elt) {
			int *mappings;
			unsigned int n_mappings;
			const char *const *xml_names;
			if (group_elt->get_name() == u8"axes") {
				mappings = axes;
				n_mappings = N_AXES;
				xml_names = nullptr;
			} else if (group_elt->get_name() == u8"buttons") {
				mappings = buttons;
				n_mappings = N_BUTTONS;
				xml_names = BUTTON_XML_NAMES;
			} else {
				std::abort();
			}
			const xmlpp::Node::NodeList &mapping_elts = group_elt->get_children();
			for (const xmlpp::Node *mapping_elt_node : mapping_elts) {
				const xmlpp::Element *mapping_elt = dynamic_cast<const xmlpp::Element *>(mapping_elt_node);
				if (mapping_elt) {
					assert(mapping_elt->get_name() == u8"mapping");
					const Glib::ustring &logs = mapping_elt->get_attribute_value(u8"logical");
					const Glib::ustring &physs = mapping_elt->get_attribute_value(u8"physical");
					assert(!logs.empty());
					assert(!physs.empty());
					int log;
					if (xml_names) {
						for (log = 0; static_cast<unsigned int>(log) < n_mappings && logs != xml_names[log]; ++log) {
						}
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
	return static_cast<unsigned int>(axes[axis]);
}

void JoystickMapping::clear_axis(unsigned int axis) {
	assert(axis < N_AXES);
	axes[axis] = -1;
}

void JoystickMapping::set_axis(unsigned int logical, unsigned int physical) {
	assert(logical < N_AXES);
	assert(physical <= 255);
	axes[logical] = static_cast<int>(physical);
}

bool JoystickMapping::has_button(unsigned int button) const {
	assert(button < N_BUTTONS);
	return buttons[button] != -1;
}

unsigned int JoystickMapping::button(unsigned int button) const {
	assert(has_button(button));
	return static_cast<unsigned int>(buttons[button]);
}

void JoystickMapping::clear_button(unsigned int button) {
	assert(button < N_BUTTONS);
	buttons[button] = -1;
}

void JoystickMapping::set_button(unsigned int logical, unsigned int physical) {
	assert(logical < N_BUTTONS);
	assert(physical <= 255);
	buttons[logical] = static_cast<int>(physical);
}

void JoystickMapping::save(xmlpp::Element *elt) const {
	assert(elt->get_name() == u8"joystick");
	elt->set_attribute(u8"name", identifier_.name);
	elt->set_attribute(u8"bus_type", std::to_string(identifier_.bus_type));
	elt->set_attribute(u8"vendor_id", std::to_string(identifier_.vendor_id));
	elt->set_attribute(u8"product_id", std::to_string(identifier_.product_id));
	elt->set_attribute(u8"version", std::to_string(identifier_.version));
	const struct {
		const char *elt_name;
		unsigned int n_mappings;
		const int *mappings;
		const char *const *xml_names;
	} groups[] = {
		{ u8"axes", N_AXES, axes, nullptr },
		{ u8"buttons", N_BUTTONS, buttons, BUTTON_XML_NAMES },
	};
	for (std::size_t i = 0; i < G_N_ELEMENTS(groups); ++i) {
		xmlpp::Element *group_elt = elt->add_child(groups[i].elt_name);
		for (unsigned int j = 0; j < groups[i].n_mappings; ++j) {
			if (groups[i].mappings[j] >= 0) {
				xmlpp::Element *mapping_elt = group_elt->add_child(u8"mapping");
				mapping_elt->set_attribute(u8"logical", groups[i].xml_names ? groups[i].xml_names[j] : todecu(j));
				mapping_elt->set_attribute(u8"physical", todecu(static_cast<unsigned int>(groups[i].mappings[j])));
			}
		}
	}
}

