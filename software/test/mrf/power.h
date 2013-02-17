#ifndef TEST_MRF_POWER_H
#define TEST_MRF_POWER_H

#include "mrf/dongle.h"
#include <memory>
#include <stdint.h>
#include <gtkmm/button.h>
#include <gtkmm/entry.h>
#include <gtkmm/table.h>

/**
 * \brief A panel that lets the user manually control power to various subsystems
 */
class PowerPanel : public Gtk::Table {
	public:
		/**
		 * \brief Constructs a new \code PowerPanel
		 *
		 * \param[in] dongle the dongle to send messages over
		 *
		 * \param[in] index the index of the robot to control
		 */
		PowerPanel(MRFDongle &dongle, unsigned int index);

	private:
		MRFDongle &dongle;
		unsigned int index;
		Gtk::Button drivetrain_button;
		Gtk::Entry drivetrain_status;
		Gtk::Entry *current_operation_status_entry;
		std::unique_ptr<MRFDongle::SendReliableMessageOperation> message;

		void power_drivetrain();
		void send_message(uint8_t code);
		void check_result(AsyncOperation<void> &op);
};

#endif

