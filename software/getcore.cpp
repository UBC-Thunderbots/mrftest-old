#include "main.h"
#include "mrf/constants.h"
#include "util/libusb.h"
#include "util/string.h"
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <locale>
#include <stdexcept>
#include <string>
#include <vector>

#define ROBOT_VID 0x0483U
#define ROBOT_PID 0x497EU
#define ROBOT_READ_CORE 0x0DU

#define CORE_SIZE (256U * 1024U)
#define BLOCK_SIZE 1024U

namespace {
	void usage(const char *appname) {
		std::cerr << "Usage:\n";
		std::cerr << appname << " {robot | dongle | flashburner} <outfile>\n";
		std::cerr << '\n';
	}

	void run_robot(const std::string &filename) {
		// Open the dongle
		std::cout << "Addressing robot… ";
		std::cout.flush();
		USB::Context ctx;
		USB::DeviceHandle devh(ctx, ROBOT_VID, ROBOT_PID);

		// Move the robot into configuration 1 (it will nearly always already be there).
		if (devh.get_configuration() != 1) {
			devh.set_configuration(1);
		}

		// Open the output file.
		std::cout << "OK\nOpening output file… ";
		std::cout.flush();
		std::ofstream ofs;
		ofs.exceptions(std::ios_base::badbit | std::ios_base::failbit);
		ofs.open(filename, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);

		// Read the data.
		std::cout << "OK\nTransferring data… ";
		std::cout.flush();
		for (uint32_t pointer = 0U; pointer < CORE_SIZE; pointer += BLOCK_SIZE) {
			char buffer[BLOCK_SIZE];
			devh.control_in(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE, ROBOT_READ_CORE, static_cast<uint16_t>(pointer / BLOCK_SIZE), 0, buffer, BLOCK_SIZE, 0);
			ofs.write(buffer, BLOCK_SIZE);
		}
		ofs.close();

		// Done!
		std::cout << "OK\n";
	}

	void run_dongle(const std::string &filename) {
		// Open the dongle
		std::cout << "Addressing dongle… ";
		std::cout.flush();
		USB::Context ctx;
		USB::DeviceHandle devh(ctx, MRF::VENDOR_ID, MRF::PRODUCT_ID, std::getenv("MRF_SERIAL"));

		// Move the dongle into configuration 1 (it will nearly always already be there).
		if (devh.get_configuration() != 1) {
			devh.set_configuration(1);
		}

		// Open the output file.
		std::cout << "OK\nOpening output file… ";
		std::cout.flush();
		std::ofstream ofs;
		ofs.exceptions(std::ios_base::badbit | std::ios_base::failbit);
		ofs.open(filename, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);

		// Read the data.
		std::cout << "OK\nTransferring data… ";
		std::cout.flush();
		for (uint32_t pointer = 0U; pointer < CORE_SIZE; pointer += BLOCK_SIZE) {
			char buffer[BLOCK_SIZE];
			devh.control_in(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE, MRF::CONTROL_REQUEST_READ_CORE, static_cast<uint16_t>(pointer / BLOCK_SIZE), 0, buffer, BLOCK_SIZE, 0);
			ofs.write(buffer, BLOCK_SIZE);
		}
		ofs.close();

		// Done!
		std::cout << "OK\n";
	}
}

int app_main(int argc, char **argv) {
	// Set the current locale from environment variables
	std::locale::global(std::locale(""));

	// Parse command-line arguments
	if (argc != 3) {
		usage(argv[0]);
		return 1;
	}
	const std::string device_type(argv[1]), filename(argv[2]);

	// Do the work.
	if (device_type == "robot") {
		run_robot(filename);
		return 0;
	} else if (device_type == "dongle") {
		run_dongle(filename);
		return 0;
	} else {
		usage(argv[0]);
		return 1;
	}
}

