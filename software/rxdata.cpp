#include "util/config.h"
#include "util/dprint.h"
#include "util/noncopyable.h"
#include "util/xbee.h"
#include "xbee/client/lowlevel.h"
#include "xbee/client/packet.h"
#include "xbee/client/raw.h"
#include "xbee/shared/packettypes.h"
#include <cstdlib>
#include <cstring>
#include <exception>
#include <fstream>
#include <glibmm.h>
#include <iostream>
#include <locale>
#include <stdexcept>

namespace {
	class StateMachine : public NonCopyable, public sigc::trackable {
		public:
			StateMachine(Glib::RefPtr<Glib::MainLoop> loop, const XBeeRawBot::Ptr bot) : loop(loop), bot(bot) {
				conn = bot->signal_alive.connect(sigc::mem_fun(this, &StateMachine::on_alive));
				bot->signal_claim_failed.connect(sigc::mem_fun(this, &StateMachine::on_claim_failed));
				std::memset(seen_indices, 0, sizeof(seen_indices));
			}

		private:
			const Glib::RefPtr<Glib::MainLoop> loop;
			const XBeeRawBot::Ptr bot;
			sigc::connection conn;
			bool seen_indices[128];
			int16_t data[128][32];

			void on_claim_failed() {
				std::cout << "Claim failed.\n";
				loop->quit();
			}

			void on_alive() {
				std::cout << "Robot claimed. Setting 16-bit address\n";
				do_set16();
			}

			void do_set16() {
				conn.disconnect();
				uint8_t value[2] = { static_cast<uint8_t>(bot->address16() >> 8), static_cast<uint8_t>(bot->address16() & 0xFF) };
				RemoteATPacket<2>::Ptr pkt(RemoteATPacket<2>::create(bot->address, "MY", value, true));
				conn = pkt->signal_complete().connect(sigc::mem_fun(this, &StateMachine::done_set16));
				bot->send(pkt);
			}

			void done_set16(const void *data, std::size_t) {
				// Check sanity.
				const XBeePacketTypes::REMOTE_AT_RESPONSE &pkt = *static_cast<const XBeePacketTypes::REMOTE_AT_RESPONSE *>(data);
				if (pkt.apiid != XBeePacketTypes::REMOTE_AT_RESPONSE_APIID) {
					return;
				}
				if (XBeeUtil::address_from_bytes(pkt.address64) != bot->address) {
					return;
				}
				if (pkt.command[0] != 'M' || pkt.command[1] != 'Y') {
					return;
				}

				// Check return status.
				switch (pkt.status) {
					case XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_OK:
						// Command executed successfully.
						// Disconnect signals.
						conn.disconnect();
						std::cout << "16-bit address assigned OK. Waiting for data.\n";
						conn = bot->signal_receive16.connect(sigc::mem_fun(this, &StateMachine::on_receive16));
						return;

					case XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_NO_RESPONSE:
						// No response from the remote system.
						// Try resending, if we have retries left.
						do_set16();
						return;

					case XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_ERROR:
						// Hard error.
						// Don't bother retrying; just report an error.
						std::cout << "Error assigning 16-bit address.\n";
						loop->quit();
						return;

					case XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_INVALID_COMMAND:
						// Hard error.
						// Don't bother retrying; just report an error.
						std::cout << "AT command rejected.\n";
						loop->quit();
						return;

					case XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_INVALID_PARAMETER:
						// Hard error.
						// Don't bother retrying; just report an error.
						std::cout << "AT command parameter rejected.\n";
						loop->quit();
						return;

					default:
						// Hard error.
						// Don't bother retrying; just report an error.
						std::cout << "Unknown error.\n";
						loop->quit();
						return;
				}
			}

#warning unportable, replace with proper encoding/decoding
			struct __attribute__((packed)) RXDATA {
				uint8_t index;
				uint16_t data[32];
			};

			void on_receive16(uint8_t, const void *data, std::size_t length) {
				// Check sanity.
				if (length != sizeof(RXDATA)) {
					std::cout << "Wrong size, got " << length << " but expected " << sizeof(RXDATA) << '\n';
					for (unsigned int i = 0; i < length; ++i) {
						std::cout << tohex(static_cast<const unsigned char *>(data)[i], 2) << ' ';
					}
					std::cout << '\n';
					return;
				}
				const RXDATA &pkt = *static_cast<const RXDATA *>(data);

				seen_indices[pkt.index] = true;
				std::memcpy(&this->data[pkt.index], pkt.data, sizeof(pkt.data));
				bool seen_all = true;
				for (unsigned int i = 0; i < 128; ++i) {
					seen_all = seen_all && seen_indices[i];
					std::cout << (seen_indices[i] ? (i == pkt.index ? '+' : 'X') : '.');
				}
				std::cout << '\n';
				if (seen_all) {
					std::ofstream ofs("/tmp/data.txt");
					for (unsigned int i = 0; i < 128; ++i) {
						for (unsigned int j = 0; j < 8; ++j) {
							for (unsigned int k = 0; k < 4; ++k) {
								if (k) {
									ofs << '\t';
								}
								ofs << this->data[i][j * 4 + k];
							}
							ofs << '\n';
						}
					}
					std::cout << "Data dumped to /tmp/data.txt.\n";
					loop->quit();
				}
			}
	};

	int main_impl(int argc, char **argv) {
		std::locale::global(std::locale(""));
		Glib::OptionContext option_context;
		option_context.set_summary("Runs the testing data dumper.");

		Glib::OptionGroup option_group("thunderbots", "Data Dumper Options", "Show Data Dumper Options");

		Glib::OptionEntry robot_entry;
		robot_entry.set_long_name("robot");
		robot_entry.set_short_name('r');
		robot_entry.set_description("Indicates the name of the robot whose data should be received");
		robot_entry.set_arg_description("ROBOT");
		Glib::ustring robot;
		option_group.add_entry(robot_entry, robot);

		option_context.set_main_group(option_group);

		if (!option_context.parse(argc, argv) || robot.empty() || argc != 1) {
			std::cerr << option_context.get_help();
			return 1;
		}

		Glib::RefPtr<Glib::MainLoop> loop(Glib::MainLoop::create());
		const Config conf;
		if (!conf.robots().contains_name(robot)) {
			std::cerr << "No such robot!\n";
			return 1;
		}
		const Config::RobotInfo &botinfo(conf.robots().find(robot));
		XBeeLowLevel modem;

		const XBeeRawBot::Ptr bot(XBeeRawBot::create(botinfo.address, modem));
		StateMachine sm(loop, bot);
		loop->run();

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

