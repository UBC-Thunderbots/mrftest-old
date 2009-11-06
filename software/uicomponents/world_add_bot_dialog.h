#ifndef UICOMPONENTS_WORLD_ADD_BOT_DIALOG_H
#define UICOMPONENTS_WORLD_ADD_BOT_DIALOG_H

#include <gtkmm.h>
#include <stdint.h>

//
// The "add bot" dialogue for the real world.
//
// The dialogue should be run with the Gtk::Dialog::run() method.
// The response will be Gtk::RESPONSE_ACCEPT if the user commits.
//
class world_add_bot_dialog : public Gtk::Dialog {
	public:
		//
		// Constructs a new "add bot" dialogue.
		//
		world_add_bot_dialog(Gtk::Window &parent);

		//
		// Gets the colour of the newly-specified bot.
		//
		bool is_yellow() const;

		//
		// Gets the name of the newly-specified bot.
		//
		Glib::ustring name() const;

		//
		// Gets the XBee address of the newly-specified bot.
		//
		uint64_t address() const;

	private:
		Gtk::HBox colour_hbox;
		Gtk::RadioButton::Group colour_group;
		Gtk::RadioButton yellow_button, blue_button;

		Gtk::HBox grid_hbox;
		Gtk::VBox grid_left_vbox, grid_right_vbox;
		Gtk::Label name_label;
		Gtk::Entry name_entry;
		Gtk::Label address_label;
		Gtk::Entry address_entry;
		Gtk::Button *ok_button;

		bool check_address(GdkEventFocus *);
};

#endif

