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
		 * \param sim the Simulator the window is displaying
		 */
		MainWindow(Simulator &sim);

	private:
		Simulator &sim;
		Gtk::ToggleButton vis_button;
		Gtk::Window vis_window;
		Visualizer vis;

		void on_vis_button_toggled();
};

#endif

