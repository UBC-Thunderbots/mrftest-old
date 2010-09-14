#ifndef SIM_MAIN_WINDOW_H
#define SIM_MAIN_WINDOW_H

#include "simulator/simulator.h"
#include "uicomponents/visualizer.h"
#include <gtkmm.h>

/**
 * The main window providing a GUI for the simulator.
 */
class MainWindow : public Gtk::Window {
	public:
		/**
		 * Constructs a new main window.
		 *
		 * \param[in] sim the Simulator the window is displaying.
		 */
		MainWindow(Simulator &sim);

	private:
		Simulator &sim;
};

#endif

