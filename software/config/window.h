#ifndef CONFIGEDITOR_WINDOW_H
#define CONFIGEDITOR_WINDOW_H

#include "util/config.h"
#include <gtkmm.h>

/**
 * The main window.
 */
class Window : public Gtk::Window {
	public:
		/**
		 * Constructs a new Window.
		 *
		 * \param[in] conf the configuration file.
		 */
		Window(Config &conf);

	private:
		Config &conf;

		virtual bool on_delete_event(GdkEventAny *);
};

#endif

