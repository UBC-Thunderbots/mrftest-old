#ifndef TEST_COMMON_DRIBBLE_H
#define TEST_COMMON_DRIBBLE_H

#include "drive/robot.h"
#include <gtkmm/box.h>
#include <gtkmm/scale.h>
#include <gtkmm/togglebutton.h>

/**
 * \brief A panel that lets the user manually control the dribbler.
 */
class DribblePanel final : public Gtk::VBox {
	public:
		/**
		 * \brief Constructs a new DribblePanel.
		 *
		 * \param[in] robot the robot to control
		 */
		explicit DribblePanel(Drive::Robot &robot);

		/**
		 * \brief Stops the dribbler.
		 */
		void stop();

		/**
		 * \brief Toggles the dribbler.
		 */
		void toggle();

	private:
		Drive::Robot &robot;
		Gtk::ToggleButton dribble_button;
		Gtk::HScale level;

		void on_update();
};

#endif


