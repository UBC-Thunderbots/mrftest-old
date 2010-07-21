#ifndef TESTER_WINDOW_H
#define TESTER_WINDOW_H

#include "test/feedback.h"
#include "test/zeroable.h"
#include "uicomponents/annunciator.h"
#include "uicomponents/single_bot_combobox.h"
#include "util/config.h"
#include "xbee/client/drive.h"
#include "xbee/client/lowlevel.h"
#include <gtkmm.h>

//
// The user interface for the tester.
//
class TesterWindow : public Gtk::Window {
	public:
		TesterWindow(XBeeLowLevel &modem, const Config &conf);

	private:
		XBeeLowLevel &modem;
		const Config &conf;
		XBeeDriveBot::Ptr bot;

		Gtk::VBox vbox;

		Gtk::Frame bot_frame;
		Gtk::HBox bot_hbox;
		SingleBotComboBox bot_chooser;
		Gtk::ToggleButton claim_bot_button;

		Gtk::Frame feedback_frame;
		TesterFeedback feedback;

		Gtk::Frame drive_frame;
		Gtk::VBox drive_box;
		Gtk::ComboBoxText drive_chooser;
		Gtk::Widget *drive_widget;
		Zeroable *drive_zeroable;

		Gtk::Frame dribble_frame;
		Gtk::HScale dribble_scale;

		Gtk::Frame chicker_frame;
		Gtk::HBox chicker_box;
		Gtk::CheckButton chicker_enabled;
		Gtk::HScale chicker_power;
		Gtk::Button chicker_kick;
		Gtk::Button chicker_chip;
		Gtk::ToggleButton chicker_autokick;
		Gtk::ToggleButton chicker_autochip;
		Light chicker_ready_light, lt3751_fault_light, chicker_low_fault_light, chicker_high_fault_light;
		Annunciator ann;

		int key_snoop(Widget *, GdkEventKey *event);
		void on_claim_toggled();
		void on_bot_alive();
		void on_bot_claim_failed_locked();
		void on_bot_claim_failed_resource();
		void on_bot_claim_failed(const Glib::ustring &);
		void drive_mode_changed();
		void on_dribble_change();
		void on_chicker_enable_change();
		void on_chicker_kick();
		void on_chicker_chip();
		void on_chicker_autokick_toggled();
		void on_chicker_autochip_toggled();
		void on_feedback();
};

#endif

