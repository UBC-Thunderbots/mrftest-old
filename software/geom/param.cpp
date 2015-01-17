#include "geom/param.h"
#include "proto/log_record.pb.h"
#include "util/string.h"
#include <locale>
#include <sstream>
#include <libxml++/libxml++.h>

RadianParam::RadianParam(const char *name, const char *location, double def, double min, double max) : NumericParam(name, location, def, min, max, false) {
}

void RadianParam::encode_value_to_log(Log::Parameter &param) const {
	param.set_radian_value(get().to_radians());
}

void RadianParam::load(const xmlpp::Element *elt) {
	if (elt->get_name() == u8"radians") {
		const xmlpp::TextNode *text_node = elt->get_child_text();
		adjustment()->set_value(std::stod(ustring2wstring(text_node->get_content())));
	}
}

void RadianParam::save(xmlpp::Element *elt) const {
	elt->set_name(u8"radians");
	elt->set_attribute(u8"name", name());
	std::wostringstream oss;
	oss.imbue(std::locale("C"));
	oss.setf(std::ios_base::fixed, std::ios_base::floatfield);
	oss.precision(static_cast<int>(fractional_digits()));
	oss << adjustment()->get_value();
	elt->set_child_text(wstring2ustring(oss.str()));
}

DegreeParam::DegreeParam(const char *name, const char *location, double def, double min, double max) : NumericParam(name, location, def, min, max, false) {
}

void DegreeParam::encode_value_to_log(Log::Parameter &param) const {
	param.set_degree_value(get().to_degrees());
}

void DegreeParam::load(const xmlpp::Element *elt) {
	if (elt->get_name() == u8"degrees") {
		const xmlpp::TextNode *text_node = elt->get_child_text();
		adjustment()->set_value(std::stod(ustring2wstring(text_node->get_content())));
	}
}

void DegreeParam::save(xmlpp::Element *elt) const {
	elt->set_name(u8"degrees");
	elt->set_attribute(u8"name", name());
	std::wostringstream oss;
	oss.imbue(std::locale("C"));
	oss.setf(std::ios_base::fixed, std::ios_base::floatfield);
	oss.precision(static_cast<int>(fractional_digits()));
	oss << adjustment()->get_value();
	elt->set_child_text(wstring2ustring(oss.str()));
}

