#include "fw/ihex.h"
#include "fw/mrf.h"
#include "fw/pk2.h"
#include "fw/xbee.h"
#include "util/crc16.h"
#include "util/string.h"
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <locale>
#include <string>
#include <glibmm/exception.h>
#include <glibmm/optioncontext.h>
#include <glibmm/optionentry.h>
#include <glibmm/optiongroup.h>

namespace {
	int main_impl(int argc, char **argv) {
		// Set the current locale from environment variables.
		std::locale::global(std::locale(""));

		// Seed the PRNGs.
		std::srand(static_cast<unsigned int>(std::time(0)));
		srand48(static_cast<long>(std::time(0)));

		// Parse the command-line arguments.
		Glib::OptionContext option_context;
		option_context.set_summary("Updates the firmware on a robot.");

		Glib::OptionGroup option_group("thunderbots", "Firmware Updater Options", "Show Firmware Updater Options");

		Glib::OptionEntry mode_option;
		mode_option.set_long_name("mode");
		mode_option.set_description("Chooses what type of file to upload and over what medium");
		Glib::ustring mode_string;
		option_group.add_entry(mode_option, mode_string);

		Glib::OptionEntry robot_option;
		robot_option.set_long_name("robot");
		robot_option.set_short_name('r');
		robot_option.set_description("Selects the robot to update");
		robot_option.set_arg_description("ROBOT");
		int robot = -1;
		option_group.add_entry(robot_option, robot);

		Glib::OptionEntry signature_option;
		signature_option.set_long_name("signature");
		signature_option.set_short_name('s');
		signature_option.set_description("Displays the build signature of the hex file (in addition to, or instead of, uploading)");
		bool signature = false;
		option_group.add_entry(signature_option, signature);

		Glib::OptionEntry hex_option;
		hex_option.set_long_name("hex");
		hex_option.set_short_name('h');
		hex_option.set_description("Selects the file to upload");
		hex_option.set_arg_description("FILE");
		std::string hex_filename;
		option_group.add_entry_filename(hex_option, hex_filename);

		option_context.set_main_group(option_group);

		if (!option_context.parse(argc, argv)) {
			std::cerr << option_context.get_help() << '\n';
			return 1;
		}

		enum {
			MODE_2011_XBEE_FPGA,
			MODE_2011_XBEE_PIC,
			MODE_2012_PK2_FPGA,
			MODE_2012_MRF_FPGA,
		} mode;
		if (mode_string == u8"2011-xbee-fpga") {
			mode = MODE_2011_XBEE_FPGA;
		} else if (mode_string == u8"2011-xbee-pic") {
			mode = MODE_2011_XBEE_PIC;
		} else if (mode_string == u8"2012-pk2-fpga" || mode_string.empty()) {
			mode = MODE_2012_PK2_FPGA;
		} else if (mode_string == u8"2012-mrf-fpga") {
			mode = MODE_2012_MRF_FPGA;
		} else {
			std::cerr << "Unrecognized mode string " << mode_string << ".\n";
			return 1;
		}
		if (!hex_filename.size()) {
			std::cerr << "--hex must be specified.\n";
			return 1;
		}

		Firmware::IntelHex hex;
		switch (mode) {
			case MODE_2011_XBEE_FPGA:
			case MODE_2012_PK2_FPGA:
			case MODE_2012_MRF_FPGA:
				hex.add_section(0, 2 * 1024 * 1024);
				break;
			case MODE_2011_XBEE_PIC:
				hex.add_section(0, 128 * 1024);
				break;
		}
		hex.load(hex_filename);

		if (signature) {
			enum {
				SIGNATURE_TYPE_CRC16,
				SIGNATURE_TYPE_CRC32,
			} signature_type;
			const std::vector<unsigned char> *signature_section;
			std::size_t signature_len;
			switch (mode) {
				case MODE_2011_XBEE_FPGA:
					signature_type = SIGNATURE_TYPE_CRC16;
					signature_section = &hex.data()[0];
					signature_len = 2 * 1024 * 1024;
					break;
				case MODE_2011_XBEE_PIC:
					signature_type = SIGNATURE_TYPE_CRC16;
					signature_section = &hex.data()[0];
					signature_len = 0x1F000;
					break;
				case MODE_2012_PK2_FPGA:
				case MODE_2012_MRF_FPGA:
					signature_type = SIGNATURE_TYPE_CRC32;
					signature_section = &hex.data()[0];
					signature_len = 2 * 1024 * 1024;
					break;
			}
			switch (signature_type) {
				case SIGNATURE_TYPE_CRC16:
					{
						uint16_t crc = CRC16::calculate(&(*signature_section)[0], std::min(signature_section->size(), signature_len));
						for (std::size_t i = signature_section->size(); i < signature_len; ++i) {
							crc = CRC16::calculate(0xFF, crc);
						}
						std::cout << "Build signature is 0x" << tohex(crc, 4) << '\n';
						break;
					}

				case SIGNATURE_TYPE_CRC32:
#warning implement
					std::cout << "CRC32 signatures not implemented yet\n";
					break;
			}
		}

		switch (mode) {
			case MODE_2011_XBEE_FPGA:
				if (0 <= robot && robot <= 15) {
					Firmware::xbee_upload(hex, true, robot);
				}
				break;

			case MODE_2011_XBEE_PIC:
				if (0 <= robot && robot <= 15) {
					Firmware::xbee_upload(hex, false, robot);
				}
				break;

			case MODE_2012_PK2_FPGA:
				Firmware::pk2_upload(hex);
				break;

			case MODE_2012_MRF_FPGA:
				if (0 <= robot && robot <= 7) {
					Firmware::mrf_upload(hex, robot);
				}
				break;
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

