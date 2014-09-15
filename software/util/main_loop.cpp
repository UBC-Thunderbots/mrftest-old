#include "util/main_loop.h"
#include "util/noncopyable.h"
#include <cassert>
#include <exception>
#include <glibmm/exceptionhandler.h>
#include <glibmm/main.h>
#include <glibmm/refptr.h>
#include <gtkmm/window.h>
#include <sigc++/connection.h>

namespace {
	class ScopedDisconnector : public NonCopyable {
		public:
			explicit ScopedDisconnector(sigc::connection conn) : conn(conn) {
			}

			~ScopedDisconnector() {
				conn.disconnect();
			}

		private:
			sigc::connection conn;
	};

	Glib::RefPtr<Glib::MainLoop> loop() {
		static Glib::RefPtr<Glib::MainLoop> obj = Glib::MainLoop::create();
		return obj;
	}

	std::exception_ptr propagating_exception;
}

void MainLoop::run() {
	static bool added_exception_handler = false;
	if (!added_exception_handler) {
		Glib::add_exception_handler(&MainLoop::quit_with_current_exception);
		added_exception_handler = true;
	}
	loop()->run();
	if (propagating_exception) {
		std::exception_ptr eptr = propagating_exception;
		propagating_exception = std::exception_ptr();
		std::rethrow_exception(eptr);
	}
}

void MainLoop::run(Gtk::Window &window) {
	sigc::connection conn = window.signal_hide().connect(&MainLoop::quit);
	ScopedDisconnector sdisc(conn);
	window.show();
	run();
}

void MainLoop::quit() {
	loop()->quit();
}

void MainLoop::quit_with_current_exception() {
	std::exception_ptr cur = std::current_exception();
	assert(cur);
	if (!propagating_exception) {
		propagating_exception = cur;
	}
	quit();
}

