#include "uicomponents/bot_chooser.h"
#include "uicomponents/world_add_bot_dialog.h"
#include "util/config.h"
#include "util/xml.h"
#include <iomanip>

bot_chooser::bot_chooser(xmlpp::Element *xmlworld, Gtk::Window &window) : xmlplayers(xmlutil::get_only_child(xmlworld, "players")), chooser(xmlplayers, window), button_box(Gtk::BUTTONBOX_SPREAD), add_button(Gtk::Stock::ADD), del_button(Gtk::Stock::DELETE) {
	pack_start(chooser, false, false);

	add_button.signal_clicked().connect(sigc::mem_fun(chooser, &chooser_combo::add_player));
	button_box.pack_start(add_button);
	del_button.signal_clicked().connect(sigc::mem_fun(chooser, &chooser_combo::del_player));
	button_box.pack_start(del_button);
	pack_start(button_box, false, false);
}

bot_chooser::~bot_chooser() {
}

sigc::signal<void, uint64_t> &bot_chooser::signal_address_changed() {
	return chooser.signal_address_changed();
}

uint64_t bot_chooser::address() const {
	return chooser.address();
}

bot_chooser::chooser_combo::item::item(const Glib::ustring &name, uint64_t address, xmlpp::Element *xml) : name(name), address(address), xml(xml) {
}

bot_chooser::chooser_combo::item::~item() {
}

Glib::ustring bot_chooser::chooser_combo::item::format() const {
	const Glib::ustring &address_string = Glib::ustring::format(std::setfill(L'0'), std::setw(16), std::hex, address);
	return Glib::ustring::compose("%1 [%2]", name, address_string);
}

bot_chooser::chooser_combo::chooser_combo(xmlpp::Element *xmlplayers, Gtk::Window &window) : xmlplayers(xmlplayers), window(window) {
	const xmlpp::Node::NodeList &children = xmlplayers->get_children("player");
	append_text("None");
	for (xmlpp::Node::NodeList::const_iterator i = children.begin(), iend = children.end(); i != iend; ++i) {
		xmlpp::Node *node = *i;
		xmlpp::Element *elem = dynamic_cast<xmlpp::Element *>(node);
		if (elem) {
			const Glib::ustring &name = elem->get_attribute_value("name");
			const Glib::ustring &address_string = elem->get_attribute_value("address");
			uint64_t address;
			{
				std::istringstream iss(Glib::locale_from_utf8(address_string));
				iss.setf(std::ios_base::hex, std::ios_base::basefield);
				iss >> address;
			}
			item itm(name, address, elem);
			items.push_back(itm);
			append_text(itm.format());
		}
	}
	set_active_text("None");
}

bot_chooser::chooser_combo::~chooser_combo() {
}

sigc::signal<void, uint64_t> &bot_chooser::chooser_combo::signal_address_changed() {
	return sig_address_changed;
}

void bot_chooser::chooser_combo::add_player() {
	world_add_bot_dialog dlg(window);
	if (dlg.run() == Gtk::RESPONSE_ACCEPT) {
		xmlpp::Element *elem = xmlplayers->add_child("player");
		elem->set_attribute("name", dlg.name());
		elem->set_attribute("address", Glib::ustring::format(std::hex, dlg.address()));
		elem->set_attribute("colour", dlg.is_yellow() ? "yellow" : "blue");
		item itm(dlg.name(), dlg.address(), elem);
		items.push_back(itm);
		append_text(itm.format());
		config::dirty();
	}
}

void bot_chooser::chooser_combo::del_player() {
	int cur = get_active_row_number();
	if (cur >= 0) {
		item itm = items[cur];
		items.erase(items.begin() + cur);
		xmlplayers->remove_child(itm.xml);
		remove_text(itm.format());
	}
}

uint64_t bot_chooser::chooser_combo::address() const {
	int cur = get_active_row_number();
	if (cur > 0) {
		return items[cur - 1].address;
	} else {
		return 0;
	}
}

void bot_chooser::chooser_combo::on_changed() {
	sig_address_changed.emit(address());
}

