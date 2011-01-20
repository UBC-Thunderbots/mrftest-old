#include "test/window.h"
#include "util/annunciator.h"
#include "util/config.h"
#include "xbee/dongle.h"
#include <gtkmm.h>
#include <iostream>
#include <locale>
#include <stdexcept>

namespace {
	void on_xbees_state_changed(XBeeDongle &dongle) {
		switch (dongle.xbees_state) {
			case XBeeDongle::XBEES_STATE_PREINIT:
			case XBeeDongle::XBEES_STATE_INIT1_0:
			case XBeeDongle::XBEES_STATE_INIT1_1:
			case XBeeDongle::XBEES_STATE_INIT1_DONE:
			case XBeeDongle::XBEES_STATE_INIT2_0:
			case XBeeDongle::XBEES_STATE_INIT2_1:
			case XBeeDongle::XBEES_STATE_RUNNING:
				return;

			case XBeeDongle::XBEES_STATE_FAIL_0:
				throw std::runtime_error("XBee 0 initialization failed");

			case XBeeDongle::XBEES_STATE_FAIL_1:
				throw std::runtime_error("XBee 1 initialization failed");
		}

		throw std::runtime_error("XBees in unknown state");
	}

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

		// Load the configuration file.
		Config config;

		// Find and enable the dongle.
		std::cout << "Finding dongle... " << std::flush;
		XBeeDongle dongle(config.out_channel(), config.in_channel());
		std::cout << "OK\n";
		dongle.xbees_state.signal_changed().connect(sigc::bind(&on_xbees_state_changed, sigc::ref(dongle)));
		on_xbees_state_changed(dongle);
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

