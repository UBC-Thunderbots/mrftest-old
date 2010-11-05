#include "fw/ihex.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <functional>
#include <stdexcept>
#include <stdint.h>
#include <string>



namespace {
	void stripws(std::string &s) {
		// Find the first non-whitespace character.
		std::string::iterator i = std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(&std::isspace)));

		// Erase everything up to and not including the first non-whitespace character.
		s.erase(s.begin(), i);

		// Erase the suffix that is whitespace.
		while (s.size() > 0 && std::isspace(s[s.size() - 1])) {
			s.erase(s.size() - 1);
		}
	}

	uint8_t decode_hex_nybble(char ch) {
		if (ch >= '0' && ch <= '9') {
			return static_cast<uint8_t>(ch - '0');
		} else if (ch >= 'A' && ch <= 'F') {
			return static_cast<uint8_t>(ch - 'A' + 0xA);
		} else if (ch >= 'a' && ch <= 'f') {
			return static_cast<uint8_t>(ch - 'a' + 0xA);
		} else {
			throw std::runtime_error("Malformed hex file!");
		}
	}

	void decode_line_data(std::vector<uint8_t> &out, const std::string &in) {
		for (std::size_t i = 0; i < in.size(); i += 2) {
			out.push_back(static_cast<uint8_t>(decode_hex_nybble(in[i]) * 16 + decode_hex_nybble(in[i + 1])));
		}
	}

	void check_checksum(const std::vector<uint8_t> &data) {
		uint8_t checksum = 0;
		for (std::vector<uint8_t>::const_iterator i = data.begin(), iend = data.end(); i != iend; ++i) {
			checksum = static_cast<uint8_t>(checksum + *i);
		}
		if (checksum != 0) {
			throw std::runtime_error("Malformed hex file!");
		}
	}
}



void IntelHex::add_section(unsigned int start, unsigned int length) {
	sections.push_back(Section(start, length));
}



void IntelHex::load(const Glib::ustring &filename) {
	// Allocate space to hold the new data.
	std::vector<std::vector<uint8_t> > new_data(sections.size());

	// Open the file.
	std::ifstream ifs(Glib::filename_from_utf8(filename).c_str());
	if (!ifs.good()) {
		throw std::runtime_error("Cannot open hex file!");
	}

	// Remember the current "base address".
	uint32_t address_base = 0;
	bool eof = false;
	while (!eof) {
		// Read a line from the hex file.
		std::string line;
		std::getline(ifs, line);

		// Check for EOF (we should have seen an EOF record in the file).
		if (ifs.eof()) {
			throw std::runtime_error("Malformed hex file!");
		}

		// Check for other I/O errors.
		if (!ifs.good()) {
			throw std::runtime_error("I/O error reading hex file!");
		}

		// Strip any whitespace.
		stripws(line);

		// Check that the line starts with a colon, and remove it.
		if (line[0] != ':') {
			throw std::runtime_error("Malformed hex file!");
		}
		line.erase(0, 1);

		// Check that the line is an even length (whole count of bytes).
		if (line.size() % 2 != 0) {
			throw std::runtime_error("Malformed hex file!");
		}

		// Decode the line into bytes.
		std::vector<uint8_t> line_data;
		decode_line_data(line_data, line);

		// Check size.
		if (line_data.size() < 5) {
			throw std::runtime_error("Malformed hex file!");
		}
		if (line_data.size() != 1U + 2U + 1U + line_data[0] + 1U) {
			throw std::runtime_error("Malformed hex file!");
		}

		// Check the checksum.
		check_checksum(line_data);

		// Handle the record.
		uint8_t data_length = line_data[0];
		unsigned int record_address = line_data[1] * 256 + line_data[2];
		uint8_t record_type = line_data[3];
		const uint8_t *record_data = &line_data[4];
		if (record_type == 0x00) {
			// Data record.
			unsigned int real_address = address_base + record_address;
			for (uint8_t i = 0; i < data_length; ++i) {
				unsigned int byte_address = real_address + i;
				bool found = false;
				for (std::size_t j = 0; j < sections.size(); ++j) {
					const Section &sec = sections[j];
					if (sec.start() <= byte_address && byte_address < sec.start() + sec.length()) {
						found = true;
						std::vector<uint8_t> &d = new_data[j];
						unsigned int offset = byte_address - sec.start();
						while (d.size() <= offset) {
							d.push_back(0xFF);
						}
						d[offset] = record_data[i];
					}
				}
				if (!found) {
					throw std::runtime_error("Data outside of section!");
				}
			}
		} else if (record_type == 0x01) {
			// EOF record.
			if (data_length != 0) {
				throw std::runtime_error("Malformed hex file!");
			}
			eof = true;
		} else if (record_type == 0x02) {
			// Extended Segment Address record.
			if (data_length != 2 || record_address != 0) {
				throw std::runtime_error("Malformed hex file!");
			}
			address_base = (record_data[1] * 256 + record_data[0]) * 16;
		} else if (record_type == 0x03) {
			// Start Segment Address record. Ignored.
		} else if (record_type == 0x04) {
			// Extended Linear Address record.
			if (data_length != 2 || record_address != 0) {
				throw std::runtime_error("Malformed hex file!");
			}
			address_base = (record_data[1] * 256 + record_data[0]) * 256;
		} else if (record_type == 0x05) {
			// Start Linear Address record. Ignored.
		} else {
			throw std::runtime_error("Malformed hex file!");
		}
	}

	// Load successful.
	the_data = new_data;
}

