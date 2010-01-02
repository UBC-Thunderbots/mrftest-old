#ifndef TESTER_DIRECT_DRIVE_H
#define TESTER_DIRECT_DRIVE_H

#include "tester/controls.h"
#include <gtkmm.h>

class tester_control_direct_drive : public Gtk::HBox, public tester_controls {
	public:
		tester_control_direct_drive();
		void encode(xbeepacket::RUN_DATA &data);
		void scram();

	private:
		Gtk::VBox column1;
		Gtk::Label drive1_label;
		Gtk::Label drive2_label;
		Gtk::Label drive3_label;
		Gtk::Label drive4_label;
		Gtk::Label drive5_label;

		Gtk::VBox column2;
		Gtk::HScale drive1_scale;
		Gtk::HScale drive2_scale;
		Gtk::HScale drive3_scale;
		Gtk::HScale drive4_scale;
		Gtk::HScale drive5_scale;
};

#endif

