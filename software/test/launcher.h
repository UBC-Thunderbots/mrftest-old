#ifndef TEST_LAUNCHER_H
#define TEST_LAUNCHER_H

#include "test/window.h"
#include "uicomponents/annunciator.h"
#include "xbee/dongle.h"
#include <gtkmm.h>
#include <memory>

/**
 * \brief A launcher window from which testers for individual robots can be launched.
 */
class TesterLauncher : public Gtk::Window {
	public:
		/**
		 * \brief Constructs a new TesterLauncher.
		 *
		 * \param[in] dongle the radio dongle to use to communicate with robots.
		 */
		TesterLauncher(XBeeDongle &dongle);

	private:
		XBeeDongle &dongle;
		Gtk::VBox vbox;
		Gtk::Table table;
		Gtk::CheckButton checkboxes[16];
		std::unique_ptr<TesterWindow> windows[16];
		GUIAnnunciator ann;

		void on_checkbox_toggled(unsigned int i);
};

#endif

