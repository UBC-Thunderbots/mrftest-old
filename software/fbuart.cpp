#include "main.h"
#include "fw/fb/constants.h"
#include "util/exception.h"
#include "util/fd.h"
#include "util/libusb.h"
#include "util/main_loop.h"
#include "util/noncopyable.h"
#include <cerrno>
#include <csignal>
#include <cstdint>
#include <iostream>
#include <locale>
#include <glibmm/convert.h>
#include <glibmm/main.h>
#include <glibmm/optioncontext.h>
#include <glibmm/optionentry.h>
#include <glibmm/optiongroup.h>
#include <glibmm/ustring.h>
#include <sigc++/functors/mem_fun.h>
#include <sys/signalfd.h>

namespace {
	void write_io(USB::DeviceHandle &dev, uint8_t levels, uint8_t directions) {
		dev.control_no_data(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, CONTROL_REQUEST_WRITE_IO, static_cast<uint16_t>((directions << 8) | levels), 0, 250);
	}

	FileDescriptor create_sigint_fd() {
		sigset_t sigs;
		sigemptyset(&sigs);
		sigaddset(&sigs, SIGINT);
		if (sigprocmask(SIG_BLOCK, &sigs, nullptr) < 0) {
			throw SystemError("sigprocmask", errno);
		}
		int intfd = signalfd(-1, &sigs, 0);
		if (intfd < 0) {
			throw SystemError("signalfd", errno);
		}
		return FileDescriptor::create_from_fd(intfd);
	}

	class UARTReceiver : public NonCopyable {
		public:
			UARTReceiver(bool start, bool kill) :
					start(start),
					kill(kill),
					sigfd(create_sigint_fd()),
					usb_context(),
					usb_handle(usb_context, FLASH_BURNER_VID, FLASH_BURNER_PID),
					usb_config_setter(usb_handle, 4),
					usb_interface_claimer(usb_handle, 0),
					serial_data_transfer(usb_handle, 1, 256, false, 0),
					errors_transfer(usb_handle, LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, CONTROL_REQUEST_GET_ERRORS, 0, 0, 1, true, 0),
					start_transfer(usb_handle, LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, CONTROL_REQUEST_WRITE_IO, (PIN_POWER << 8) | PIN_POWER, 0, 250),
					start_release_power_transfer(usb_handle, LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, CONTROL_REQUEST_WRITE_IO, 0, 0, 250),
					kill_transfer(usb_handle, LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, CONTROL_REQUEST_WRITE_IO, PIN_POWER << 8, 0, 250),
					kill_release_power_transfer(usb_handle, LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, CONTROL_REQUEST_WRITE_IO, 0, 0, 250) {
				Glib::signal_io().connect(sigc::mem_fun(this, &UARTReceiver::handle_sigint), sigfd.fd(), Glib::IO_IN);
				serial_data_transfer.signal_done.connect(sigc::mem_fun(this, &UARTReceiver::handle_serial_data));
				errors_transfer.signal_done.connect(sigc::mem_fun(this, &UARTReceiver::handle_errors));
				start_transfer.signal_done.connect(sigc::mem_fun(this, &UARTReceiver::handle_start_done));
				start_release_power_transfer.signal_done.connect(sigc::mem_fun(this, &UARTReceiver::handle_start_released));
				kill_transfer.signal_done.connect(sigc::mem_fun(this, &UARTReceiver::handle_kill_done));
				kill_release_power_transfer.signal_done.connect(sigc::mem_fun(this, &UARTReceiver::handle_kill_released));

				serial_data_transfer.retry_on_stall(false);
				serial_data_transfer.submit();
				if (start) {
					start_transfer.submit();
				}
			}

			~UARTReceiver() {
				release_timeout_conn.disconnect();
			}

		private:
			bool start, kill;
			FileDescriptor sigfd;
			USB::Context usb_context;
			USB::DeviceHandle usb_handle;
			USB::ConfigurationSetter usb_config_setter;
			USB::InterfaceClaimer usb_interface_claimer;
			USB::BulkInTransfer serial_data_transfer;
			USB::ControlInTransfer errors_transfer;
			USB::ControlNoDataTransfer start_transfer, start_release_power_transfer, kill_transfer, kill_release_power_transfer;
			sigc::connection release_timeout_conn;

			bool handle_sigint(Glib::IOCondition) {
				if (kill) {
					kill_transfer.submit();
				} else {
					MainLoop::quit();
				}
				return false;
			}

			void handle_serial_data(AsyncOperation<void> &) {
				try {
					serial_data_transfer.result();
					std::cout.write(reinterpret_cast<const char *>(serial_data_transfer.data()), static_cast<std::streamsize>(serial_data_transfer.size()));
					std::cout.flush();
					serial_data_transfer.submit();
				} catch (const USB::TransferStallError &) {
					errors_transfer.submit();
				}
			}

			void handle_errors(AsyncOperation<void> &) {
				errors_transfer.result();
				uint8_t errors = errors_transfer.data()[0];
				if (!errors) {
					throw std::runtime_error("Protocol error: CONTROL_REQUEST_GET_ERRORS returned zero!");
				}
				if (errors & 0xF0) {
					throw std::runtime_error(Glib::locale_from_utf8(Glib::ustring::compose(u8"Protocol error: CONTROL_REQUEST_GET_ERRORS returned value %1 with reserved bits set!", static_cast<unsigned int>(errors))));
				}
				if (errors & 0x08) {
					std::cerr << "<SO>";
				}
				if (errors & 0x04) {
					std::cerr << "<UO>";
				}
				if (errors & 0x01) {
					std::cerr << "<FE>";
				}
				if (errors & 0x02) {
					std::cerr << "<ND>";
				}
				std::cerr.flush();
				try {
					usb_handle.clear_halt_in(1);
					serial_data_transfer.submit();
				} catch (const USB::TransferStallError &) {
					errors_transfer.submit();
				}
			}

			void handle_start_done(AsyncOperation<void> &) {
				start_transfer.result();
				release_timeout_conn = Glib::signal_timeout().connect(sigc::mem_fun(this, &UARTReceiver::handle_start_delay), 500);
			}

			bool handle_start_delay() {
				start_release_power_transfer.submit();
				return false;
			}

			void handle_start_released(AsyncOperation<void> &) {
				start_release_power_transfer.result();
			}

			void handle_kill_done(AsyncOperation<void> &) {
				kill_transfer.result();
				release_timeout_conn = Glib::signal_timeout().connect(sigc::mem_fun(this, &UARTReceiver::handle_kill_delay), 300);
			}

			bool handle_kill_delay() {
				kill_release_power_transfer.submit();
				return false;
			}

			void handle_kill_released(AsyncOperation<void> &) {
				kill_release_power_transfer.result();
				MainLoop::quit();
			}
	};
}

int app_main(int argc, char **argv) {
	// Set the current locale from environment variables.
	std::locale::global(std::locale(""));

	// Parse command-line options.
	Glib::OptionContext option_context;
	option_context.set_summary(u8"Runs the Thunderbots Flash burner UART receiver.");

	Glib::OptionGroup option_group(u8"thunderbots", u8"Flash burner UART receiver options", u8"Show Flash burner UART receiver options");

	Glib::OptionEntry start_entry;
	start_entry.set_long_name(u8"start");
	start_entry.set_short_name('s');
	start_entry.set_description(u8"Powers the target on startup");
	bool start = false;
	option_group.add_entry(start_entry, start);

	Glib::OptionEntry kill_entry;
	kill_entry.set_long_name(u8"kill");
	kill_entry.set_short_name('k');
	kill_entry.set_description(u8"Shuts down the target on application shutdown");
	bool kill = false;
	option_group.add_entry(kill_entry, kill);

	option_context.set_main_group(option_group);

	if (!option_context.parse(argc, argv)) {
		std::cerr << "Invalid command-line option(s). Use --help for help.\n";
		return 1;
	}

	// Create and run the main handler state machine object.
	UARTReceiver rx(start, kill);
	MainLoop::run();
	return 0;
}

