#ifndef TEST_MRF_POWER_H
#define TEST_MRF_POWER_H

#include "mrf/dongle.h"
#include <cstdint>
#include <memory>
#include <gtkmm/button.h>
#include <gtkmm/buttonbox.h>
#include <sigc++/connection.h>

/**
 * \brief A panel that lets the user manually control power to various subsystems
 */
class PowerPanel final : public Gtk::HButtonBox {
	public:
		/**
		 * \brief Constructs a new PowerPanel
		 *
		 * \param[in] dongle the dongle to send messages over
		 *
		 * \param[in] index the index of the robot to control
		 */
		explicit PowerPanel(MRFDongle &dongle, unsigned int index);

	private:
		MRFDongle &dongle;
		unsigned int index;
		Gtk::Button power_drivetrain_button, reboot_button, shut_down_button, *current_action_button;
		std::unique_ptr<MRFDongle::SendReliableMessageOperation> message;

		void power_drivetrain();
		void reboot();
		void shut_down();
		void send_message(uint8_t code, Gtk::Button &button);
		void check_result(AsyncOperation<void> &op);
		void timeout();
		void set_buttons_sensitive(bool sensitive);
};

#endif
