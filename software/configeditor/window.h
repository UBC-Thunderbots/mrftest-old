#ifndef CONFIGEDITOR_WINDOW_H
#define CONFIGEDITOR_WINDOW_H

#include "util/config.h"
#include <gtkmm.h>

//
// The main window.
//
class Window : public Gtk::Window {
	public:
		Window(Config &conf);

	private:
		Config &conf;

		virtual bool on_delete_event(GdkEventAny *);
};

#endif

