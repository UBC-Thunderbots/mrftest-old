#include "xbee/daemon/frontend/already_running.h"
#include "xbee/daemon/frontend/daemon.h"
#include "xbee/daemon/physical/packetproto.h"
#include <exception>
#include <fcntl.h>
#include <glibmm.h>
#include <iostream>
#include <locale>
#include <stdexcept>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace {
	void invoke_quit(Glib::RefPtr<Glib::MainLoop> main_loop) {
		main_loop->quit();
	}

	int main_impl(int argc, char **argv) {
		// Create a GLib main loop object.
		std::locale::global(std::locale(""));
		Glib::RefPtr<Glib::MainLoop> main_loop(Glib::MainLoop::create());

		// Parse command-line options.
		Glib::OptionEntry daemon_entry;
		daemon_entry.set_long_name("daemon");
		daemon_entry.set_short_name('d');
		daemon_entry.set_description("Dæmonizes the multiplexer after initialization.");
		bool daemonize = false;
		Glib::OptionGroup opt_group("xbeed", "Main Options");
		opt_group.add_entry(daemon_entry, daemonize);
		Glib::OptionContext opt_context;
		opt_context.set_summary("Launches the XBee multiplexer.");
		opt_context.set_main_group(opt_group);
		if (!opt_context.parse(argc, argv)) {
			return 1;
		}
		if (argc != 1) {
			std::cout << opt_context.get_help();
			return 1;
		}

		// Open the serial port.
		XBeePacketStream pstream;

		// Create the main application.
		XBeeDaemon d(pstream);

		// Configure the serial port.
		pstream.configure_port();

		// Connect to the signal so that when the last client disconnects, we will terminate.
		d.signal_last_client_disconnected.connect(sigc::bind(&invoke_quit, main_loop));

		// If we're supposed to dæmonize, then do.
		if (daemonize) {
			pid_t cpid = fork();
			if (cpid < 0) {
				throw std::runtime_error("Cannot fork!");
			} else if (cpid > 0) {
				_exit(0);
			}
			setsid();
			if (chdir("/") < 0) {
				throw std::runtime_error("Cannot move to root directory!");
			}
			const FileDescriptor::Ptr null_fd(FileDescriptor::create_open("/dev/null", O_RDWR, 0));
			if (dup2(null_fd->fd(), 0) < 0 || dup2(null_fd->fd(), 1) < 0 || dup2(null_fd->fd(), 2) < 0) {
				throw std::runtime_error("Cannot redirect standard streams to /dev/null!");
			}
		}

		// Go into the main loop.
		main_loop->run();

		// Exit the application.
		return 0;
	}
}

int main(int argc, char **argv) {
	try {
		return main_impl(argc, argv);
	} catch (const AlreadyRunning &exp) {
		// No need to show a message for this case.
		return 0;
	} catch (const Glib::Exception &exp) {
		std::cerr << exp.what() << '\n';
	} catch (const std::exception &exp) {
		std::cerr << exp.what() << '\n';
		return 1;
	} catch (...) {
		std::cerr << "Unknown error!\n";
		return 1;
	}
}

