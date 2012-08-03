#include "util/libusb.h"
#include "util/string.h"
#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
	unsigned long long convert_number(const std::string &str) {
		std::size_t end_pos = 0;
		unsigned long long ull = std::stoull(str, &end_pos, 0);
		if (end_pos != str.size()) {
			throw std::runtime_error("Whole string not consumed in number conversion");
		}
		return ull;
	}

	int main_impl(int argc, char **argv) {
		// Set the current locale from environment variables
		std::locale::global(std::locale(""));

		// Parse command-line arguments
		if (argc != 7 && argc != 8) {
			std::cerr << "Usage:\n";
			std::cerr << argv[0] << " <channel> <symbol-rate> <pan-id> <mac-address> <capture-flags> <capture-file> [<dongle-serial#>]\n";
			std::cerr << '\n';
			std::cerr << "  <channel> is the channel on which to capture, a number from 0x0B to 0x1A\n";
			std::cerr << "  <symbol-rate> is the bit rate at which to capture, either 250 or 625\n";
			std::cerr << "  <pan-id> is the PAN ID to capture (if PAN ID filtering is enabled), a number from 0x0001 to 0xFFFE\n";
			std::cerr << "  <mac-address> is the station’s local MAC address, a 64-bit number excluding 0, 0xFFFF, and 0xFFFFFFFFFFFFFFFF\n";
			std::cerr << "  <capture-flags> is a numerical combination of capture flags described at <http://trac.thecube.ca/trac/thunderbots/wiki/Electrical/RadioProtocol/2012/USB#SetPromiscuousFlags>\n";
			std::cerr << "  <capture-file> is the name of the .pcap file to write the captured packets into\n";
			std::cerr << "  <dongle-serial#> is the serial number of the radio dongle to enable for capture\n";
			return 1;
		}
		uint8_t channel = static_cast<uint8_t>(convert_number(argv[1]));
		unsigned int symbol_rate = static_cast<unsigned int>(convert_number(argv[2]));
		if (symbol_rate != 250 && symbol_rate != 625) {
			std::cerr << "Invalid symbol rate; must be one of 250 or 625\n";
			return 1;
		}
		uint8_t symbol_rate_encoded = symbol_rate == 625 ? 1 : 0;
		uint16_t pan_id = static_cast<uint16_t>(convert_number(argv[3]));
		uint64_t mac_address = static_cast<uint64_t>(convert_number(argv[4]));
		uint16_t capture_flags = static_cast<uint16_t>(convert_number(argv[5]));
		const char *dongle_serial = argc == 8 ? argv[7] : 0;

		// Open the dongle
		std::cout << "Addressing dongle… ";
		std::cout.flush();
		USB::Context ctx;
		USB::DeviceHandle devh(ctx, 0xC057, 0x2579, dongle_serial);
		std::cout << "OK\n";

		// Set configuration 1 to enter parameters
		if (devh.get_configuration() != 1) {
			devh.set_configuration(1);
		}
		devh.claim_interface(0);

		// Set parameters
		std::cout << "Setting channel 0x" << tohex(channel, 2) << "… ";
		std::cout.flush();
		devh.control_no_data(LIBUSB_REQUEST_TYPE_VENDOR, 0x01, channel, 0, 0);
		std::cout << "OK\nSetting symbol rate " << (symbol_rate_encoded ? 625 : 250) << " kb/s… ";
		std::cout.flush();
		devh.control_no_data(LIBUSB_REQUEST_TYPE_VENDOR, 0x03, symbol_rate_encoded, 0, 0);
		std::cout << "OK\nSetting PAN ID 0x" << tohex(pan_id, 4) << "… ";
		std::cout.flush();
		devh.control_no_data(LIBUSB_REQUEST_TYPE_VENDOR, 0x05, pan_id, 0, 0);
		std::cout << "OK\nSetting MAC address 0x" << tohex(mac_address, 16) << "… ";
		std::cout.flush();
		{
			uint8_t buffer[8];
			for (std::size_t i = 0; i < 8; ++i) {
				buffer[i] = static_cast<uint8_t>(mac_address >> (i * 8));
			}
			devh.control_out(LIBUSB_REQUEST_TYPE_VENDOR, 0x07, 0, 0, buffer, sizeof(buffer), 0);
		}
		std::cout << "OK\n";

		// Set configuration 3 to enter promiscuous mode
		devh.release_interface(0);
		devh.set_configuration(3);
		devh.claim_interface(0);

		// Set capture flags
		std::cout << "Setting capture flags… ";
		std::cout.flush();
		devh.control_no_data(LIBUSB_REQUEST_TYPE_VENDOR, 0x0B, capture_flags, 0, 0);
		std::cout << "OK\n";

		// Open the capture file
		std::cout << "Opening capture file… ";
		std::cout.flush();
		std::ofstream ofs;
		ofs.exceptions(std::ios_base::eofbit | std::ios_base::failbit | std::ios_base::badbit);
		ofs.open(argv[6], std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);
		{
			uint32_t u32;
			uint16_t u16;
			u32 = 0xA1B2C3D4;
			ofs.write(reinterpret_cast<const char *>(&u32), 4); // Magic number
			u16 = 2;
			ofs.write(reinterpret_cast<const char *>(&u16), 2); // Major version number
			u16 = 4;
			ofs.write(reinterpret_cast<const char *>(&u16), 2); // Minor version number
			u32 = 0;
			ofs.write(reinterpret_cast<const char *>(&u32), 4); // Timezone offset (we use UTC)
			ofs.write(reinterpret_cast<const char *>(&u32), 4); // Significant figures in timestamps (everyone sets this to zero)
			u32 = 131;
			ofs.write(reinterpret_cast<const char *>(&u32), 4); // Maximum length of a captured packet
			u32 = 195;
			ofs.write(reinterpret_cast<const char *>(&u32), 4); // Network data link type
		}
		ofs.flush();
		std::cout << "OK\n";

		// Go into a loop
		for (;;) {
			uint8_t buffer[256];
			std::size_t len = devh.bulk_in(1, buffer, sizeof(buffer), 0);
			if (len >= 4) {
				uint32_t seconds, microseconds;
				{
					auto stamp = std::chrono::system_clock::now();
					std::time_t converted = std::chrono::system_clock::to_time_t(stamp);
					std::chrono::duration<uint64_t, std::micro> micros = stamp.time_since_epoch();
					if (static_cast<uint64_t>(converted) == micros.count() / UINT64_C(1000000)) {
						// std::chrono::system_clock and time_t use the same units on this platform, so we can get a fractional part.
						seconds = static_cast<uint32_t>(micros.count() / UINT64_C(1000000));
						microseconds = static_cast<uint32_t>(micros.count() % UINT64_C(1000000));
					} else {
						// std::chrono::system_clock and time_t use different units on this platform, so we can only achieve second resolution.
						seconds = static_cast<uint32_t>(converted);
						microseconds = 0;
					}
				}

				{
					uint32_t u32;
					u32 = static_cast<uint32_t>(seconds);
					ofs.write(reinterpret_cast<const char *>(&u32), 4);
					u32 = static_cast<uint32_t>(microseconds);
					ofs.write(reinterpret_cast<const char *>(&u32), 4);
					u32 = static_cast<uint32_t>(len - 4);
					ofs.write(reinterpret_cast<const char *>(&u32), 4);
					ofs.write(reinterpret_cast<const char *>(&u32), 4);
				}
				ofs.write(reinterpret_cast<const char *>(buffer + 2), static_cast<std::streamsize>(len - 4));
				ofs.flush();
				std::cout << "Captured packet of length " << len << '\n';
			} else {
				std::cout << "Bad capture size " << len << " (must be ≥4)\n";
			}
		}
	}
}

int main(int argc, char **argv) {
	try {
		return main_impl(argc, argv);
	} catch (const std::exception &exp) {
		std::cerr << exp.what() << '\n';
	} catch (...) {
		std::cerr << "Unknown error!\n";
	}
	return 1;
}

