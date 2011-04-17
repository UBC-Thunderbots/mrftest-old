#ifndef AI_WINDOW_H
#define AI_WINDOW_H

#include "ai/ai.h"
#include "uicomponents/visualizer.h"
#include <gtkmm.h>

namespace AI {
	/**
	 * A window for controlling the AI.
	 */
	class Window : public Gtk::Window {
		public:
			/**
			 * Creates a new main window.
			 *
			 * \param[in] ai the AI to observe and control.
			 */
			Window(AIPackage &ai);
	};
}

#endif

