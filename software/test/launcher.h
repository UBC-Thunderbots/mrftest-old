#ifndef TEST_LAUNCHER_H
#define TEST_LAUNCHER_H

#include "test/mapper.h"
#include "test/window.h"
#include "uicomponents/annunciator.h"
#include "xbee/dongle.h"
#include <memory>
#include <gtkmm/box.h>
#include <gtkmm/table.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/window.h>

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
		Gtk::ToggleButton robot_toggles[16];
		std::unique_ptr<TesterWindow> windows[16];
		Gtk::ToggleButton mapper_toggle;
		std::unique_ptr<MapperWindow> mapper_window;
		GUIAnnunciator ann;

		void on_robot_toggled(unsigned int i);
		void on_mapper_toggled();
};

#endif

