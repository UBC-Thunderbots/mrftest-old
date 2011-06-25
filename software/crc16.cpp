#include "util/crc16.h"
#include "util/string.h"
#include <fstream>
#include <iostream>
#include <locale>

namespace {
	int main_impl(int argc, char **argv) {
		// Set the current locale from environment variables.
		std::locale::global(std::locale(""));

		// Check for proper command-line arguments.
		if (argc != 2) {
			std::cerr << "Usage:\n  " << argv[0] << " FILENAME\n\nDisplays the CRC16 of a raw binary file.\n\nApplication Options:\n  FILENAME     The name of the file to checksum.\n\n";
			return 1;
		}

		// Read and CRC the file.
		std::fstream fs;
		fs.exceptions(std::ios_base::badbit);
		fs.open(argv[1], std::ios_base::in | std::ios_base::binary);
		uint16_t crc = CRC16::INITIAL;
		do {
			char buffer[4096];
			fs.read(buffer, sizeof(buffer));
			crc = CRC16::calculate(buffer, fs.gcount(), crc);
		} while (!fs.eof());
		std::cout << "0x" << tohex(crc, 4) << '\n';

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

