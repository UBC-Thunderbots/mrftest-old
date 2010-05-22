#ifndef SIM_MAIN_WINDOW_H
#define SIM_MAIN_WINDOW_H

#include "sim/simulator.h"
#include "uicomponents/visualizer.h"
#include <gtkmm.h>

/**
 * The main window providing a GUI for the simulator.
 */
class main_window : public Gtk::Window {
	public:
		/**
		 * Constructs a new main window.
		 * \param sim the simulator the window is displaying
		 */
		main_window(simulator &sim);

	private:
		simulator &sim;
		Gtk::ToggleButton vis_button;
		Gtk::Window vis_window;
		visualizer vis;

		void on_vis_button_toggled();
};

#endif

