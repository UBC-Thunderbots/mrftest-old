#include "uicomponents/world_add_bot_dialog.h"

world_add_bot_dialog::world_add_bot_dialog(Gtk::Window &parent) : Gtk::Dialog("Add Bot", parent, true), yellow_button(colour_group, "Yellow"), blue_button(colour_group, "Blue"), name_label("Name:"), address_label("XBee Address:") {
	colour_hbox.pack_start(yellow_button, true, true);
	colour_hbox.pack_start(blue_button, true, true);
	get_vbox()->pack_start(colour_hbox, false, false);

	grid_left_vbox.pack_start(name_label, false, false);
	grid_right_vbox.pack_start(name_entry, false, false);
	grid_left_vbox.pack_start(address_label, false, false);
	grid_right_vbox.pack_start(address_entry, false, false);
	grid_hbox.pack_start(grid_left_vbox, false, false);
	grid_hbox.pack_start(grid_right_vbox, true, true);
	get_vbox()->pack_start(grid_hbox, false, false);

	ok_button = add_button(Gtk::Stock::OK, Gtk::RESPONSE_ACCEPT);
	add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);

	address_entry.set_text("0000000000000000");
	address_entry.set_width_chars(16);
	address_entry.set_max_length(16);

	show_all();
}

bool world_add_bot_dialog::is_yellow() const {
	return yellow_button.get_active();
}

Glib::ustring world_add_bot_dialog::name() const {
	return name_entry.get_text();
}

uint64_t world_add_bot_dialog::address() const {
	const Glib::ustring &str = address_entry.get_text();
	std::istringstream iss(Glib::locale_from_utf8(str));
	iss.setf(std::ios_base::hex, std::ios_base::basefield);
	uint64_t address;
	iss >> address;
	return address;
}

bool world_add_bot_dialog::check_address(GdkEventFocus *) {
	const Glib::ustring &str = address_entry.get_text();
	if (str.size() != 16) {
		ok_button->set_sensitive(false);
		return true;
	}
	bool valid = true;
	for (unsigned int i = 0; i < str.size(); i++)
		if (!std::isxdigit(str[i]))
			valid = false;
	ok_button->set_sensitive(valid);
	return true;
}

