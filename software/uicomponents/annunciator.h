#ifndef UICOMPONENTS_ANNUNCIATOR_H
#define UICOMPONENTS_ANNUNCIATOR_H

#include <gtkmm.h>

/**
 * A graphical panel that displays annunciator messages.
 */
class GUIAnnunciator : public Gtk::ScrolledWindow {
	public:
		/**
		 * Constructs a new annunciator panel ready to add to a window.
		 */
		GUIAnnunciator();

		/**
		 * Destroys an annunciator panel.
		 */
		~GUIAnnunciator();
};

#endif

