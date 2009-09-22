#include "simulator/window.h"
#include <gtkmm.h>

class simulator_window_impl : public virtual Gtk::Window {
	public:
		simulator_window_impl();

	protected:
		virtual bool on_delete_event(GdkEventAny *);
};

simulator_window_impl::simulator_window_impl() : Gtk::Window() {
	show_all();
}

bool simulator_window_impl::on_delete_event(GdkEventAny *) {
	Gtk::Main::quit();
	return true;
}

simulator_window::simulator_window() : impl(new simulator_window_impl) {
}

simulator_window::~simulator_window() {
	delete impl;
}

