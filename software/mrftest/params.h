#ifndef MRFTEST_PARAMS_H
#define MRFTEST_PARAMS_H

#include "mrf/robot.h"
#include <memory>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/buttonbox.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/entry.h>
#include <gtkmm/label.h>
#include <gtkmm/scale.h>
#include <gtkmm/table.h>

/**
 * \brief A panel that allows the user to edit the operational parameters of a robot
 */
class ParamsPanel : public Gtk::Table {
	public:
		/**
		 * \brief Constructs a new ParamsPanel
		 *
		 * \param[in] dongle the dongle talking to the robot
		 *
		 * \param[in] robot the robot whose parameters should be edited
		 */
		ParamsPanel(MRFDongle &dongle, MRFRobot &robot);

	private:
		MRFDongle &dongle;
		MRFRobot &robot;
		Gtk::Label channel_label;
		Gtk::ComboBoxText channel_chooser;
		Gtk::Label index_label;
		Gtk::ComboBoxText index_chooser;
		Gtk::Label pan_label;
		Gtk::Entry pan_entry;
		Gtk::VBox vbox;
		Gtk::HButtonBox hbb;
		Gtk::Button set, reboot;
		std::unique_ptr<MRFDongle::SendReliableMessageOperation> message;
		bool rebooting;

		void activate_controls(bool act = true);
		void send_params();
		void reboot_robot();
		void check_result(AsyncOperation<void> &op);
};

#endif

