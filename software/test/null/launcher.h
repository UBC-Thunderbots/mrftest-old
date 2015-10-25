#ifndef TEST_NULL_LAUNCHER_H
#define TEST_NULL_LAUNCHER_H

#include "test/common/mapper.h"
#include "test/null/window.h"
#include "drive/null/dongle.h"
#include "uicomponents/annunciator.h"
#include <memory>
#include <gtkmm/box.h>
#include <gtkmm/table.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/window.h>

/**
 * \brief A launcher window from which testers for individual robots can be launched.
 */
class NullTesterLauncher final : public Gtk::Window {
	public:
		/**
		 * \brief Constructs a new TesterLauncher.
		 *
		 * \param[in] dongle the radio dongle to use to communicate with robots.
		 */
		explicit NullTesterLauncher(Drive::Null::Dongle &dongle);

	private:
		Drive::Null::Dongle &dongle;
		Gtk::VBox vbox;
		Gtk::ToggleButton robot_toggle;
		std::unique_ptr<NullTesterWindow> window;
		Gtk::ToggleButton mapper_toggle;
		std::unique_ptr<MapperWindow> mapper_window;
		GUIAnnunciator ann;

		void on_robot_toggled();
		void on_mapper_toggled();
};

#endif
