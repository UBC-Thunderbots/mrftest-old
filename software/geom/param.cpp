#include "geom/param.h"
#include "proto/log_record.pb.h"
#include "util/string.h"
#include <iomanip>
#include <locale>
#include <libxml++/libxml++.h>

RadianParam::RadianParam(const char *name, const char *location, double def, double min, double max) : NumericParam(name, location, def, min, max, false) {
}

void RadianParam::encode_value_to_log(Log::Parameter &param) const {
	param.set_radian_value(get().to_radians());
}

void RadianParam::load(const xmlpp::Element *elt) {
	if (elt->get_name() == u8"radians") {
		const xmlpp::TextNode *text_node = elt->get_child_text();
		std::wistringstream iss(ustring2wstring(text_node->get_content()));
		iss.imbue(std::locale("C"));
		double v;
		iss >> v;
		adjustment()->set_value(v);
	}
}

void RadianParam::save(xmlpp::Element *elt) const {
	elt->set_name(u8"radians");
	elt->set_attribute(u8"name", name());
	std::wostringstream oss;
	oss.imbue(std::locale("C"));
	oss << std::fixed << std::setprecision(static_cast<int>(fractional_digits())) << adjustment()->get_value();
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
		std::wistringstream iss(ustring2wstring(text_node->get_content()));
		iss.imbue(std::locale("C"));
		double v;
		iss >> v;
		adjustment()->set_value(v);
	}
}

void DegreeParam::save(xmlpp::Element *elt) const {
	elt->set_name(u8"degrees");
	elt->set_attribute(u8"name", name());
	std::wostringstream oss;
	oss.imbue(std::locale("C"));
	oss << std::fixed << std::setprecision(static_cast<int>(fractional_digits())) << adjustment()->get_value();
	elt->set_child_text(wstring2ustring(oss.str()));
}

