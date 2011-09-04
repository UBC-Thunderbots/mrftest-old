#ifndef UICOMPONENTS_ANNUNCIATOR_H
#define UICOMPONENTS_ANNUNCIATOR_H

#include <gtkmm.h>

/**
 * A graphical panel that displays annunciator messages.
 */
class GUIAnnunciator : public Gtk::ScrolledWindow {
	public:
		/**
		 * \brief Constructs a new annunciator panel ready to add to a window.
		 */
		GUIAnnunciator();

	private:
		Gtk::TreeView view;
		Gtk::CellRendererText message_renderer;
};

#endif

