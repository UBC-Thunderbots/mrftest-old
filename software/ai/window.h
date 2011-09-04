#ifndef AI_WINDOW_H
#define AI_WINDOW_H

#include "ai/ai.h"
#include "ai/param_panel.h"
#include "uicomponents/annunciator.h"
#include "uicomponents/visualizer.h"
#include <gtkmm.h>

namespace AI {
	/**
	 * \brief A window for controlling the AI.
	 */
	class Window : public Gtk::Window {
		public:
			/**
			 * \brief Creates a new main window.
			 *
			 * \param[in] ai the AI to observe and control.
			 */
			Window(AIPackage &ai);

		private:
			Gtk::VBox outer_vbox;
			Gtk::HPaned hpaned;
			Gtk::Frame notebook_frame;
			Gtk::Notebook notebook;
			Gtk::VBox main_vbox, secondary_vbox;
			Gtk::Frame secondary_basics_frame, secondary_visualizer_controls_frame;
			ParamPanel param_panel;
			Gtk::VPaned vpaned;
			Visualizer visualizer;
			GUIAnnunciator annunciator;
	};
}

#endif

