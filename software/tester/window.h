#ifndef TESTER_WINDOW_H
#define TESTER_WINDOW_H

#include "tester/feedback.h"
#include "tester/zeroable.h"
#include "uicomponents/single_bot_combobox.h"
#include "util/config.h"
#include "xbee/client/drive.h"
#include "xbee/client/lowlevel.h"
#include <gtkmm.h>

//
// The user interface for the tester.
//
class tester_window : public Gtk::Window {
	public:
		tester_window(xbee_lowlevel &modem, const config &conf);

	private:
		xbee_lowlevel &modem;
		xbee_drive_bot::ptr bot;

		Gtk::VBox vbox;

		Gtk::Frame bot_frame;
		Gtk::HBox bot_hbox;
		single_bot_combobox bot_chooser;
		Gtk::ToggleButton claim_bot_button;

		Gtk::Frame feedback_frame;
		tester_feedback feedback;

		Gtk::Frame drive_frame;
		Gtk::VBox drive_box;
		Gtk::ComboBoxText drive_chooser;
		Gtk::Widget *drive_widget;
		zeroable *drive_zeroable;

		Gtk::Frame dribble_frame;
		Gtk::HScale dribble_scale;

		Gtk::Frame chicker_frame;
		Gtk::HBox chicker_box;
		Gtk::CheckButton chicker_enabled;
		Gtk::Button chicker_kick;
		Gtk::Button chicker_chip;
		light chicker_status;

		int key_snoop(Widget *, GdkEventKey *event);
		void on_claim_toggled();
		void on_bot_alive();
		void on_bot_dead();
		void on_bot_claim_failed_locked();
		void on_bot_claim_failed_resource();
		void on_bot_claim_failed(const Glib::ustring &);
		void drive_mode_changed();
		void on_dribble_change();
		void on_chicker_enable_change();
		void on_chicker_kick();
		void on_chicker_chip();
		void on_feedback();
};

#endif

