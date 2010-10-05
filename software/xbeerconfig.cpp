#include "util/config.h"
#include "util/dprint.h"
#include "util/noncopyable.h"
#include "xbee/client/lowlevel.h"
#include <cstdlib>
#include <cstring>
#include <exception>
#include <glibmm.h>
#include <iostream>
#include <locale>
#include <stdexcept>

namespace {
	class ResponseHolder : public NonCopyable {
		public:
			ResponseHolder(const Glib::RefPtr<Glib::MainLoop> loop) : length(0), response(0), loop(loop) {
			}

			~ResponseHolder() {
				delete[] static_cast<char *>(response);
			}

			void handle_response(const void *resp, std::size_t len) {
				delete[] static_cast<char *>(response);
				length = len;
				response = new char[length];
				std::memcpy(response, resp, length);
				loop->quit();
			}

			std::size_t length;
			void *response;

		private:
			const Glib::RefPtr<Glib::MainLoop> loop;
	};

	class NoResponseError : public std::exception {
		public:
			NoResponseError() {
			}

			const char *what() const throw () {
				return "Peer did not respond to remote AT command.";
			}
	};

	void build_param(unsigned int param, uint8_t *parambytes, std::size_t length) {
		for (unsigned int i = 0; i < length; ++i) {
			parambytes[i] = (param >> ((length - i - 1) * 8)) & 0xFF;
		}
	}

	template<std::size_t length> unsigned int do_local_at_command(const Glib::RefPtr<Glib::MainLoop> loop, XBeeLowLevel &modem, const Glib::ustring &message, const char *command, unsigned int param) {
		std::cout << message << "... " << std::flush;
		uint8_t parambytes[length];
		build_param(param, parambytes, length);
		XBeePacket::Ptr pkt(ATPacket<length>::create(command, parambytes));
		ResponseHolder resph(loop);
		pkt->signal_complete().connect(sigc::mem_fun(resph, &ResponseHolder::handle_response));
		modem.send(pkt);
		loop->run();
		const XBeePacketTypes::AT_RESPONSE &resp(*static_cast<const XBeePacketTypes::AT_RESPONSE *>(resph.response));
		if (resp.command[0] != command[0] || resp.command[1] != command[1]) {
			std::cout << "FAIL\n";
			throw std::runtime_error("Modem returned response to wrong local AT command.");
		} else if (resp.status == XBeePacketTypes::AT_RESPONSE_STATUS_OK) {
			std::cout << "OK\n";
			unsigned int retval = 0;
			for (unsigned int i = 0; i < resph.length - sizeof(XBeePacketTypes::AT_RESPONSE); ++i) {
				retval = (retval << 8) | resp.value[i];
			}
			return retval;
		} else if (resp.status == XBeePacketTypes::AT_RESPONSE_STATUS_ERROR) {
			std::cout << "FAIL\n";
			throw std::runtime_error("Modem returned general error to local AT command.");
		} else if (resp.status == XBeePacketTypes::AT_RESPONSE_STATUS_INVALID_COMMAND) {
			std::cout << "FAIL\n";
			throw std::runtime_error("Modem indicated invalid local AT command.");
		} else if (resp.status == XBeePacketTypes::AT_RESPONSE_STATUS_INVALID_PARAMETER) {
			std::cout << "FAIL\n";
			throw std::runtime_error("Modem indicated invalid parameter value to local AT command.");
		} else {
			std::cout << "FAIL\n";
			throw std::runtime_error("Modem returned unknown response to local AT command.");
		}
	}

	template<std::size_t length> unsigned int do_remote_at_command(const Glib::RefPtr<Glib::MainLoop> loop, uint64_t bot, XBeeLowLevel &modem, const Glib::ustring &message, const char *command, unsigned int param, bool apply, bool permit_no_response = false) {
		std::cout << message << "... " << std::flush;
		uint8_t parambytes[length];
		build_param(param, parambytes, length);
		XBeePacket::Ptr pkt(RemoteATPacket<length>::create(bot, command, parambytes, apply));
		ResponseHolder resph(loop);
		pkt->signal_complete().connect(sigc::mem_fun(resph, &ResponseHolder::handle_response));
		modem.send(pkt);
		loop->run();
		const XBeePacketTypes::REMOTE_AT_RESPONSE &resp(*static_cast<const XBeePacketTypes::REMOTE_AT_RESPONSE *>(resph.response));
		if (resp.command[0] != command[0] || resp.command[1] != command[1]) {
			std::cout << "FAIL\n";
			throw std::runtime_error("Modem returned response to wrong remote AT command.");
		} else if (resp.status == XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_OK) {
			std::cout << "OK\n";
			unsigned int retval = 0;
			for (unsigned int i = 0; i < resph.length - sizeof(XBeePacketTypes::AT_RESPONSE); ++i) {
				retval = (retval << 8) | resp.value[i];
			}
			return retval;
		} else if (resp.status == XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_ERROR) {
			std::cout << "FAIL\n";
			throw std::runtime_error("Modem returned general error to local AT command.");
		} else if (resp.status == XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_INVALID_COMMAND) {
			std::cout << "FAIL\n";
			throw std::runtime_error("Modem indicated invalid local AT command.");
		} else if (resp.status == XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_INVALID_PARAMETER) {
			std::cout << "FAIL\n";
			throw std::runtime_error("Modem indicated invalid parameter value to local AT command.");
		} else if (resp.status == XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_NO_RESPONSE) {
			if (permit_no_response) {
				std::cout << "OK\n";
				return 0;
			} else {
				std::cout << "FAIL\n";
				throw NoResponseError();
			}
		} else {
			std::cout << "FAIL\n";
			throw std::runtime_error("Modem returned unknown response to local AT command.");
		}
	}

	bool do_work(Glib::RefPtr<Glib::MainLoop> loop, uint64_t bot, XBeeLowLevel &modem, unsigned int target) {
		// First phase, search for the modem on all channels and both PAN IDs.
		static const unsigned int PANIDS[] = { XBeePacketTypes::THUNDERBOTS_PANID, XBeePacketTypes::FACTORY_PANID };
		bool found = false;
		for (unsigned int i = 0; i < G_N_ELEMENTS(PANIDS) && !found; ++i) {
			do_local_at_command<2>(loop, modem, Glib::ustring::compose("Switching local modem to PAN ID %1", tohex(PANIDS[i], 4)), "ID", PANIDS[i]);
			for (unsigned int j = XBeePacketTypes::MIN_CHANNEL; j <= XBeePacketTypes::MAX_CHANNEL && !found; ++j) {
				do_local_at_command<1>(loop, modem, Glib::ustring::compose("Switching local modem to channel %1", tohex(j, 2)), "CH", j);
				try {
					unsigned int version = do_remote_at_command<0>(loop, bot, modem, "Getting remote firmware version", "VR", 0, true);
					if (version < 0x10E6) {
						std::cout << "WARNING --- WARNING --- WARNING\nFirmware version is less than 10E6. Consider upgrading!\nWARNING --- WARNING --- WARNING\n";
					}
					found = true;
				} catch (const NoResponseError &) {
					// Swallow.
				}
			}
		}

		if (!found) {
			return false;
		}

		// Configure the modem.
		do_remote_at_command<0>(loop, bot, modem, "Getting remote firmware version", "VR", 0, true);
		do_remote_at_command<1>(loop, bot, modem, "Setting API mode with escapes", "AP", 2, false);
		do_remote_at_command<4>(loop, bot, modem, "Setting serial baud rate to 250,000", "BD", 250000, false);
		do_remote_at_command<1>(loop, bot, modem, "Enabling RTS flow control", "D6", 1, false);
		do_remote_at_command<1>(loop, bot, modem, "Setting DIO0 to default as digital output low", "D0", 4, false);
		do_remote_at_command<1>(loop, bot, modem, Glib::ustring::compose("Setting radio channel to %1", tohex(target, 2)), "CH", target, false);
		do_remote_at_command<2>(loop, bot, modem, "Setting PAN ID to 496C", "ID", 0x496C, false);
		do_remote_at_command<2>(loop, bot, modem, "Setting 16-bit address to none", "MY", 0xFFFF, false);
		do_remote_at_command<1>(loop, bot, modem, "Setting clear channel threshold to -44dBm", "CA", 0x2C, false);
		do_remote_at_command<0>(loop, bot, modem, "Saving and applying configuration", "WR", 0, true);

		return true;
	}

	int main_impl(int argc, char **argv) {
		std::locale::global(std::locale(""));
		Glib::OptionContext option_context;
		option_context.set_summary("Runs the remote configurator utility.");

		Glib::OptionGroup option_group("thunderbots", "Remote Configurator Options", "Show Remote Configurator Options");

		Glib::OptionEntry robot_entry;
		robot_entry.set_long_name("robot");
		robot_entry.set_short_name('r');
		robot_entry.set_description("Indicates the name of the robot whose XBee should be configured");
		robot_entry.set_arg_description("ROBOT");
		int robot = -1;
		option_group.add_entry(robot_entry, robot);

		option_context.set_main_group(option_group);

		if (!option_context.parse(argc, argv) || robot < 0 || argc != 1) {
			std::cerr << option_context.get_help();
			return 1;
		}

		Glib::RefPtr<Glib::MainLoop> loop(Glib::MainLoop::create());
		const Config conf;
		if (!conf.robots().contains_pattern(robot)) {
			std::cerr << "No such robot!\n";
			return 1;
		}
		const Config::RobotInfo &botinfo(conf.robots().find(robot));
		XBeeLowLevel modem;

		try {
			bool success = do_work(loop, botinfo.address, modem, conf.channel());
			do_local_at_command<0>(loop, modem, "Rebooting local modem", "FR", 0);
			if (success) {
				std::cout << "===================\nOPERATION SUCCEEDED\n===================\n";
			}
			return success ? 0 : 1;
		} catch (...) {
			try {
				do_local_at_command<0>(loop, modem, "Rebooting local modem", "FR", 0);
			} catch (...) {
				// Swallow.
			}
			throw;
		}

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

