#ifndef TEST_MRF_LEDS_H
#define TEST_MRF_LEDS_H

#include "mrf/dongle.h"
#include <memory>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/entry.h>

/**
 * \brief A panel that lets the user control the LEDs
 */
class LEDsPanel : public Gtk::VBox {
	public:
		/**
		 * \brief Constructs a new LEDsPanel
		 *
		 * \param[in] dongle the dongle to talk over
		 *
		 * \param[in] index the index of the robot to control
		 */
		explicit LEDsPanel(MRFDongle &dongle, unsigned int index);

	private:
		MRFDongle &dongle;
		unsigned int index;
		Gtk::ComboBoxText mode_chooser;
		Gtk::Button set_button;
		Gtk::Entry status_entry;
		std::unique_ptr<MRFDongle::SendReliableMessageOperation> message;

		void set_mode();
		void check_result(AsyncOperation<void> &op);
};

#endif

