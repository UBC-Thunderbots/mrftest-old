#ifndef SIM_MAIN_WINDOW_H
#define SIM_MAIN_WINDOW_H

#include "sim/simulator.h"
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
};

#endif

