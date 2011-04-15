#include "test/window.h"
#include "util/annunciator.h"
#include "xbee/dongle.h"
#include <gtkmm.h>
#include <iostream>
#include <locale>
#include <stdexcept>

namespace {
	class EnableRadiosOperation : public sigc::trackable {
		public:
			EnableRadiosOperation(XBeeDongle &dongle) : dongle(dongle), main_loop(Glib::MainLoop::create()) {
				std::cout << "Enabling radios... " << std::flush;
				dongle.enable()->signal_done.connect(sigc::mem_fun(this, &EnableRadiosOperation::on_radios_enabled));
			}

			~EnableRadiosOperation() {
			}

			void run() {
				main_loop->run();
			}

		private:
			XBeeDongle &dongle;
			Glib::RefPtr<Glib::MainLoop> main_loop;

			void on_radios_enabled(AsyncOperation<void>::Ptr op) {
				op->result();
				std::cout << "OK\n";
				main_loop->quit();
			}
	};

	int main_impl(int argc, char **argv) {
		// Set the current locale from environment variables.
		std::locale::global(std::locale(""));

		// Parse the command-line arguments.
		Glib::OptionContext option_context;
		option_context.set_summary("Allows testing robot subsystems.");

		// Initialize GTK.
		Gtk::Main m(argc, argv, option_context);
		if (argc != 1) {
			std::cout << option_context.get_help();
			return 1;
		}

		// Find and enable the dongle.
		std::cout << "Finding dongle... " << std::flush;
		XBeeDongle dongle;
		std::cout << "OK\n";
		{
			EnableRadiosOperation op(dongle);
			op.run();
		}

		// Create the window.
		TesterWindow win(dongle);
		Gtk::Main::run(win);

		return 0;
	}
}

int main(int argc, char **argv) {
	try {
		return main_impl(argc, argv);
	} catch (const Glib::Exception &exp) {
		std::cerr << exp.what() << '\n';
	} catch (const std::exception &exp) {
		std::cerr << exp.what() << '\n';
	} catch (...) {
		std::cerr << "Unknown error!\n";
	}
	return 1;
}

