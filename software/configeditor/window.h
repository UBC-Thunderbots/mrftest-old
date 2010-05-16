#ifndef CONFIGEDITOR_WINDOW_H
#define CONFIGEDITOR_WINDOW_H

#include "util/config.h"
#include <gtkmm.h>

//
// The main window.
//
class window : public Gtk::Window {
	public:
		window(config &conf);

	private:
		config &conf;

		virtual bool on_delete_event(GdkEventAny *);
};

#endif

