#ifndef TESTER_PERMOTOR_DRIVE_H
#define TESTER_PERMOTOR_DRIVE_H

#include "xbee/bot.h"
#include <gtkmm.h>

class tester_control_permotor_drive : public Gtk::HBox {
	public:
		tester_control_permotor_drive();
		void set_robot(radio_bot::ptr bot);
		void zero();
		virtual void drive(int16_t m1, int16_t m2, int16_t m3, int16_t m4) = 0;

	protected:
		radio_bot::ptr robot;

	private:
		Gtk::VBox column1;
		Gtk::Label drive1_label;
		Gtk::Label drive2_label;
		Gtk::Label drive3_label;
		Gtk::Label drive4_label;

		Gtk::VBox column2;
		Gtk::HScale drive1_scale;
		Gtk::HScale drive2_scale;
		Gtk::HScale drive3_scale;
		Gtk::HScale drive4_scale;

		void on_change();
};

#endif

