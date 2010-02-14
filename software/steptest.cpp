#include "util/args.h"
#include "xbee/packettypes.h"
#include "xbee/xbee.h"
#include <algorithm>
#include <iostream>
#include <gtkmm.h>

namespace {
	class step_tester : public noncopyable {
		public:
			step_tester() {
				std::fill(received, received + sizeof(received) / sizeof(*received), false);
				modem.signal_received().connect(sigc::mem_fun(*this, &step_tester::on_receive));
			}

			void run() {
				Gtk::Main::run();
			}

		private:
			xbee modem;
			int16_t data[1024];
			bool received[1024];

			void on_receive(const void *dataptr, std::size_t) {
				const xbeepacket::RECEIVE_HDR &hdr = *static_cast<const xbeepacket::RECEIVE_HDR *>(dataptr);
				if (hdr.apiid != xbeepacket::RECEIVE_APIID) return;
				const uint8_t *payloadptr = static_cast<const uint8_t *>(dataptr);
				payloadptr += sizeof(hdr);
				uint16_t start_address = (payloadptr[0] << 8) | payloadptr[1];
				std::cerr << "Received " << start_address << ':';
				for (unsigned int offset = 0; start_address + offset < sizeof(data) / sizeof(*data) && offset < 48; ++offset) {
					data[start_address + offset] = (payloadptr[2 + 2 * offset] << 8) | payloadptr[2 + 2 * offset + 1];
					received[start_address + offset] = true;
					std::cerr << ' ' << data[start_address + offset];
				}
				std::cerr << '\n';
				bool found = false;
				for (unsigned int i = 0; i < sizeof(data) / sizeof(*data); ++i) {
					if (!received[i]) {
						std::cerr << "Unseen: " << i << '\n';
						found = true;
						break;
					}
				}
				if (!found) {
					std::cerr << "Done!\n";
					for (unsigned int i = 0; i < sizeof(data) / sizeof(*data); ++i) {
						std::cout << data[i] << '\n';
					}
					Gtk::Main::quit();
				}
			}
	};
}

int main(int argc, char **argv) {
	args::argc = argc;
	args::argv = argv;

	Gtk::Main mn(argc, argv);
	step_tester tester;
	tester.run();
}

