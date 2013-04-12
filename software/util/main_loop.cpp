#include "util/main_loop.h"
#include <glibmm/main.h>
#include <glibmm/refptr.h>
#include <gtkmm/window.h>
#include <sigc++/connection.h>

namespace {
	Glib::RefPtr<Glib::MainLoop> loop() {
		static Glib::RefPtr<Glib::MainLoop> obj = Glib::MainLoop::create();
		return obj;
	}
}

void MainLoop::run() {
	loop()->run();
}

void MainLoop::run(Gtk::Window &window) {
	sigc::connection conn = window.signal_hide().connect(&MainLoop::quit);
	window.show();
	run();
	conn.disconnect();
}

void MainLoop::quit() {
	loop()->quit();
}

