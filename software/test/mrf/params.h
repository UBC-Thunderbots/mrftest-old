#ifndef TEST_MRF_PARAMS_H
#define TEST_MRF_PARAMS_H

#include "mrf/dongle.h"
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
#include <sigc++/connection.h>

/**
 * \brief A panel that allows the user to edit the operational parameters of a robot
 */
class ParamsPanel : public Gtk::Table {
	public:
		/**
		 * \brief Constructs a new ParamsPanel.
		 *
		 * \param[in] dongle the dongle talking to the robot
		 *
		 * \param[in] robot the robot whose parameters should be edited
		 */
		explicit ParamsPanel(MRFDongle &dongle, MRFRobot &robot);

		/**
		 * \brief Destroys a ParamsPanel.
		 */
		~ParamsPanel();

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
		Gtk::Button set, reboot, shut_down;
		std::unique_ptr<MRFDongle::SendReliableMessageOperation> message;
		bool rebooting, shutting_down;
		sigc::connection reset_button_connection;

		void activate_controls(bool act = true);
		void send_params();
		void reboot_robot();
		void shut_down_robot();
		void check_result(AsyncOperation<void> &op);
		void reset_button_text();
};

#endif

