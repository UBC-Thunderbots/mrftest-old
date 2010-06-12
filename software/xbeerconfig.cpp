#include "util/config.h"
#include "util/noncopyable.h"
#include "xbee/client/lowlevel.h"
#include <glibmm.h>
#include <iomanip>
#include <iostream>
#include <clocale>
#include <cstdlib>
#include <cstring>

namespace {
	class response_holder : public noncopyable {
		public:
			response_holder(const Glib::RefPtr<Glib::MainLoop> loop) : length(0), response(0), loop(loop) {
			}

			~response_holder() {
				delete [] static_cast<char *>(response);
			}

			void handle_response(const void *resp, std::size_t len) {
				delete [] static_cast<char *>(response);
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

	template<std::size_t length>
	void set_parameter(const Glib::RefPtr<Glib::MainLoop> loop, uint64_t bot, xbee_lowlevel &modem, const Glib::ustring &message, const char *command, unsigned int param, bool apply) {
		std::cout << message << "... " << std::flush;
		uint8_t parambytes[length];
		for (unsigned int i = 0; i < length; ++i) {
			parambytes[i] = (param >> ((length - i - 1) * 8)) & 0xFF;
		}
		packet::ptr pkt(remote_at_packet<length>::create(bot, command, parambytes, apply));
		response_holder resph(loop);
		pkt->signal_complete().connect(sigc::mem_fun(resph, &response_holder::handle_response));
		modem.send(pkt);
		loop->run();
		const xbeepacket::REMOTE_AT_RESPONSE &resp(*static_cast<const xbeepacket::REMOTE_AT_RESPONSE *>(resph.response));
		if (resp.command[0] != command[0] || resp.command[1] != command[1]) {
			std::cout << "FAIL (wrong command, " << resp.command[0] << resp.command[1] << ")\n";
			std::abort();
		}
		if (resp.status == xbeepacket::REMOTE_AT_RESPONSE_STATUS_NO_RESPONSE && command[0] == 'R' && command[1] == 'E') {
			std::cout << "OK\n";
			return;
		}
		if (resp.status != xbeepacket::REMOTE_AT_RESPONSE_STATUS_OK) {
			std::cout << "FAIL ";
			if (resp.status == xbeepacket::REMOTE_AT_RESPONSE_STATUS_ERROR) {
				std::cout << "(general error)";
			} else if (resp.status == xbeepacket::REMOTE_AT_RESPONSE_STATUS_INVALID_COMMAND) {
				std::cout << "(invalid command)";
			} else if (resp.status == xbeepacket::REMOTE_AT_RESPONSE_STATUS_INVALID_PARAMETER) {
				std::cout << "(invalid parameter)";
			} else if (resp.status == xbeepacket::REMOTE_AT_RESPONSE_STATUS_NO_RESPONSE) {
				std::cout << "(no response)";
			} else {
				std::cout << "(unknown error)";
			}
			std::cout << '\n';
			std::abort();
		}
		std::cout << "OK\n";
	}

	bool try_channel(const Glib::RefPtr<Glib::MainLoop> loop, uint64_t bot, xbee_lowlevel &modem, unsigned int channel, unsigned int target, void (*executable)(Glib::RefPtr<Glib::MainLoop>, uint64_t, xbee_lowlevel &, unsigned int)) {
		response_holder resph(loop);

		{
			std::cout << "Switching local XBee to channel " << Glib::ustring::format(std::hex, std::setw(2), std::setfill(L'0'), std::uppercase, channel) << "... " << std::flush;
			uint8_t channel_byte = channel;
			packet::ptr pkt(at_packet<1>::create("CH", &channel_byte));
			pkt->signal_complete().connect(sigc::mem_fun(resph, &response_holder::handle_response));
			modem.send(pkt);
			loop->run();
			const xbeepacket::AT_RESPONSE &resp(*static_cast<const xbeepacket::AT_RESPONSE *>(resph.response));
			if (resp.command[0] != 'C' || resp.command[1] != 'H') {
				std::cout << "FAIL\n";
				std::abort();
			}
			if (resp.status != xbeepacket::AT_RESPONSE_STATUS_OK) {
				std::cout << "FAIL\n";
				std::abort();
			}
			std::cout << "OK\n";
		}

		{
			std::cout << "Getting firmware version... " << std::flush;
			packet::ptr pkt(remote_at_packet<0>::create(bot, "VR", 0, true));
			pkt->signal_complete().connect(sigc::mem_fun(resph, &response_holder::handle_response));
			modem.send(pkt);
			loop->run();
			const xbeepacket::REMOTE_AT_RESPONSE &resp(*static_cast<const xbeepacket::REMOTE_AT_RESPONSE *>(resph.response));
			if (resp.command[0] != 'V' || resp.command[1] != 'R') {
				std::cout << "FAIL\n";
				std::abort();
			}
			if (resp.status == xbeepacket::REMOTE_AT_RESPONSE_STATUS_NO_RESPONSE) {
				std::cout << "FAIL\n";
				return false;
			}
			if (resp.status != xbeepacket::REMOTE_AT_RESPONSE_STATUS_OK) {
				std::cout << "FAIL\n";
				std::abort();
			}
			if (resph.length != sizeof(xbeepacket::REMOTE_AT_RESPONSE) + 2) {
				std::cout << "FAIL\n";
				std::abort();
			}
			std::cout << "OK\n";
			unsigned int version = (resp.value[0] << 8) | resp.value[1];
			if (version < 0x10E6) {
				std::cout << "WARNING --- WARNING --- WARNING\nFirmware version is less than 10E6. Consider upgrading!\nWARNING --- WARNING --- WARNING\n";
			}
		}

		(*executable)(loop, bot, modem, target);

		return true;
	}

	bool try_pan(Glib::RefPtr<Glib::MainLoop> loop, uint64_t bot, xbee_lowlevel &modem, uint16_t panid, unsigned int target, void (*executable)(Glib::RefPtr<Glib::MainLoop>, uint64_t, xbee_lowlevel &, unsigned int)) {
		response_holder resph(loop);
		std::cout << "Switching local XBee to PAN " << Glib::ustring::format(std::hex, std::setw(4), std::setfill(L'0'), std::uppercase, panid) << "... " << std::flush;
		uint8_t pan_bytes[2] = { static_cast<uint8_t>(panid >> 8), static_cast<uint8_t>(panid & 0xFF) };
		packet::ptr pkt(at_packet<2>::create("ID", &pan_bytes));
		pkt->signal_complete().connect(sigc::mem_fun(resph, &response_holder::handle_response));
		modem.send(pkt);
		loop->run();
		const xbeepacket::AT_RESPONSE &resp(*static_cast<const xbeepacket::AT_RESPONSE *>(resph.response));
		if (resp.command[0] != 'I' || resp.command[1] != 'D') {
			std::cout << "FAIL\n";
			std::abort();
		}
		if (resp.status != xbeepacket::AT_RESPONSE_STATUS_OK) {
			std::cout << "FAIL\n";
			std::abort();
		}
		std::cout << "OK\n";

		for (unsigned int chan = 0x0B; chan <= 0x1A; ++chan) {
			if (try_channel(loop, bot, modem, chan, target, executable)) {
				return true;
			}
		}
		return false;
	}

	bool run_executable(Glib::RefPtr<Glib::MainLoop> loop, uint64_t bot, xbee_lowlevel &modem, unsigned int target, void (*executable)(Glib::RefPtr<Glib::MainLoop>, uint64_t, xbee_lowlevel &, unsigned int)) {
		const uint16_t pans[] = { 0x3332, 0x496C };
		for (unsigned int i = 0; i < sizeof(pans) / sizeof(*pans); ++i) {
			if (try_pan(loop, bot, modem, pans[i], target, executable)) {
				return true;
			}
		}
		return false;
	}

	void load_factory_settings(Glib::RefPtr<Glib::MainLoop> loop, uint64_t bot, xbee_lowlevel &modem, unsigned int) {
		set_parameter<0>(loop, bot, modem, "Loading factory default settings", "RE", 0, true);
	}

	void set_custom_parameters(Glib::RefPtr<Glib::MainLoop> loop, uint64_t bot, xbee_lowlevel &modem, unsigned int target) {
		set_parameter<1>(loop, bot, modem, "Setting API mode with escapes", "AP", 2, false);
		set_parameter<4>(loop, bot, modem, "Setting serial baud rate to 250,000", "BD", 250000, false);
		set_parameter<1>(loop, bot, modem, "Enabling RTS flow control", "D6", 1, false);
		set_parameter<1>(loop, bot, modem, "Setting DIO0 to default as digital output low", "D0", 4, false);
		set_parameter<1>(loop, bot, modem, Glib::ustring::compose("Setting radio channel to %1", Glib::ustring::format(std::hex, std::setw(2), std::setfill(L'0'), std::uppercase, target)), "CH", target, false);
		set_parameter<2>(loop, bot, modem, "Setting PAN ID to 496C", "ID", 0x496C, false);
		set_parameter<2>(loop, bot, modem, "Setting 16-bit address to none", "MY", 0xFFFF, false);
		set_parameter<0>(loop, bot, modem, "Saving and applying configuration", "WR", 0, true);
	}

	int main_impl(int argc, char **argv) {
		Glib::RefPtr<Glib::MainLoop> loop(Glib::MainLoop::create());
		Glib::OptionContext option_context;
		option_context.set_summary("Runs the remote configurator utility.");

		Glib::OptionGroup option_group("thunderbots", "Remote Configurator Options", "Show Remote Configurator Options");

		Glib::OptionEntry robot_entry;
		robot_entry.set_long_name("robot");
		robot_entry.set_short_name('r');
		robot_entry.set_description("Indicates the name of the robot whose XBee should be configured");
		Glib::ustring robot;
		option_group.add_entry(robot_entry, robot);

		option_context.set_main_group(option_group);

		if (!option_context.parse(argc, argv) || robot.empty()) {
			std::cerr << option_context.get_help();
			return 1;
		}

		const config conf;
		if (!conf.robots().contains_name(robot)) {
			std::cerr << "No such robot!\n";
			return 1;
		}
		const config::robot_info &botinfo(conf.robots().find(robot));
		xbee_lowlevel modem;

		void (* const executables[])(Glib::RefPtr<Glib::MainLoop>, uint64_t, xbee_lowlevel &, unsigned int) = { &load_factory_settings, &set_custom_parameters };
		for (unsigned int i = 0; i < sizeof(executables) / sizeof(*executables); ++i) {
			if (!run_executable(loop, botinfo.address, modem, conf.channel(), executables[i])) {
				return 1;
			}
		}

		return 0;
	}
}

int main(int argc, char **argv) {
	std::setlocale(LC_ALL, "");
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

