#ifndef TEST_MRF_MANUAL_COMMUTATION_WINDOW_H
#define TEST_MRF_MANUAL_COMMUTATION_WINDOW_H

#include "mrf/dongle.h"
#include <cstdint>
#include <memory>
#include <gtkmm/box.h>
#include <gtkmm/entry.h>
#include <gtkmm/frame.h>
#include <gtkmm/radiobutton.h>
#include <gtkmm/radiobuttongroup.h>
#include <gtkmm/table.h>
#include <gtkmm/window.h>

/**
 * \brief A window that allows the user to manually control commutation of individual motor phases.
 */
class ManualCommutationWindow : public Gtk::Window {
	public:
		/**
		 * \brief Creates a new ManualCommutationWindow.
		 *
		 * \param[in] dongle the dongle to communicate over.
		 *
		 * \param[in] index the robot index to control.
		 */
		ManualCommutationWindow(MRFDongle &dongle, unsigned int index);

	private:
		MRFDongle &dongle;
		unsigned int index;

		uint8_t old_data[5];

		Gtk::VBox vbox;
		Gtk::Frame motor_frames[5];
		Gtk::Table motor_tables[5];
		Gtk::RadioButtonGroup phase_button_groups[5][3];
		Gtk::RadioButton phase_buttons[5][3][4];
		Gtk::Entry status_entry;

		std::unique_ptr<MRFDongle::SendReliableMessageOperation> message;

		void send_packet();
		void check_result(AsyncOperation<void> &op);
		void set_controls_sensitive(bool sensitive);
};

#endif

