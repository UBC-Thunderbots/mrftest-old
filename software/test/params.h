#ifndef TEST_PARAMS_H
#define TEST_PARAMS_H

#include "xbee/robot.h"
#include <gtkmm.h>

class TesterParamsPanel : public Gtk::Table {
	public:
		TesterParamsPanel();
		~TesterParamsPanel();
		void set_robot(XBeeRobot::Ptr bot);

	private:
		XBeeRobot::Ptr robot;
		XBeeRobot::OperationalParameters::FlashContents flash_contents;
		Gtk::ComboBoxText channels[2];
		Gtk::ComboBoxText index;
		Gtk::HScale dribble_power;
		Gtk::Button commit, rollback, reboot;
		sigc::connection alive_connection;
		Gtk::Entry test_mode;
		Gtk::Button set_test_mode;
		bool freeze;

		void activate_controls(bool act = true);
		void on_alive_changed();
		void on_read_done(AsyncOperation<XBeeRobot::OperationalParameters>::Ptr op);
		void on_change();
		void on_change_done(AsyncOperation<void>::Ptr op);
		void on_commit();
		void on_commit_done(AsyncOperation<void>::Ptr op);
		void on_rollback();
		void on_reboot();
		void on_reboot_done(AsyncOperation<void>::Ptr op);
		void on_set_test_mode();
};

#endif

