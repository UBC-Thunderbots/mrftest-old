#include "main.h"
#include "util/crc32.h"
#include "util/string.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <numeric>

namespace {
	void usage(const char *appname) {
		std::cerr << "Usage:\n";
		std::cerr << appname << " <binfile>\n";
		std::cerr << '\n';
	}
}

int app_main(int argc, char **argv) {
	// Set the current locale from environment variables
	std::locale::global(std::locale(""));

	// Parse command-line arguments
	if (argc != 2) {
		usage(argv[0]);
		return 1;
	}
	const std::string filename(argv[1]);

	// Do the work.
	std::ifstream ifs;
	ifs.exceptions(std::ios_base::badbit);
	ifs.open(filename, std::ios_base::in | std::ios_base::binary);
	uint32_t crc = std::accumulate(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>(), CRC32::INITIAL, static_cast<uint32_t(*)(uint32_t, uint8_t)>(&CRC32::calculate));
	std::cout << "0x" << tohex(crc, 8) << '\n';
	return 0;
}

