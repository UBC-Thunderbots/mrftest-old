#ifndef TESTER_MATRIX_DRIVE_H
#define TESTER_MATRIX_DRIVE_H

#include "xbee/bot.h"
#include <gtkmm.h>

class tester_control_matrix_drive : public Gtk::HBox {
	public:
		tester_control_matrix_drive();
		void set_robot(radio_bot::ptr bot);
		void zero();

	protected:
		radio_bot::ptr robot;

	private:
		Gtk::VBox column1;
		Gtk::Label drive1_label;
		Gtk::Label drive2_label;
		Gtk::Label drive3_label;

		Gtk::VBox column2;
		Gtk::HScale drive1_scale;
		Gtk::HScale drive2_scale;
		Gtk::HScale drive3_scale;

		void on_change();
};

#endif

