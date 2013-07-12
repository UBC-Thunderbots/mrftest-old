#include "main.h"
#include "fw/ihex.h"
#include "fw/fb/fb.h"
#include "util/crc16.h"
#include "util/string.h"
#include <iostream>
#include <locale>
#include <string>
#include <glibmm/optioncontext.h>
#include <glibmm/optionentry.h>
#include <glibmm/optiongroup.h>

int app_main(int argc, char **argv) {
	// Set the current locale from environment variables.
	std::locale::global(std::locale(""));

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
		MODE_2013_FB_FPGA,
	} mode;
	bool onboard = false;
	bool leave_powered = false;
	if (mode_string == u8"2013-fb-fpga" || mode_string.empty()) {
		mode = MODE_2013_FB_FPGA;
	} else if (mode_string == u8"2013-fb-fpga-power") {
		mode = MODE_2013_FB_FPGA;
		leave_powered = true;
	} else if (mode_string == u8"2013-fb-onboard") {
		mode = MODE_2013_FB_FPGA;
		onboard = true;
	} else {
		std::cerr << "Unrecognized mode string " << mode_string << ".\n";
		std::cerr << "Valid modes are:\n";
		std::cerr << "2013-fb-fpga: FPGA bitstream for 2013 robots is burnt into SPI Flash using the dedicated Flash burner board.\n";
		std::cerr << "2013-fb-fpga-power: FPGA bitstream for 2013 robots is burnt into SPI Flash using the dedicated Flash burner board, leaving the robot powered afterwards.\n";
		std::cerr << "2013-fb-onboard: FPGA bitstream for 2013 robots is burnt into onboard storage of the dedicated Flash burner board, for later autonomous burning.\n";
		return 1;
	}
	if (!hex_filename.size()) {
		std::cerr << "--hex must be specified.\n";
		return 1;
	}

	Firmware::IntelHex hex;
	switch (mode) {
		case MODE_2013_FB_FPGA:
			hex.add_section(0, 2 * 1024 * 1024);
			break;
	}
	hex.load(hex_filename);

	if (signature) {
		enum {
			SIGNATURE_TYPE_CRC32,
		} signature_type;
		const std::vector<unsigned char> *signature_section;
		std::size_t signature_len;
		switch (mode) {
			case MODE_2013_FB_FPGA:
				signature_type = SIGNATURE_TYPE_CRC32;
				signature_section = &hex.data()[0];
				signature_len = 2 * 1024 * 1024;
				break;
		}
		switch (signature_type) {
			case SIGNATURE_TYPE_CRC32:
#warning implement
				std::cout << "CRC32 signatures not implemented yet\n";
				break;
		}
	}

	switch (mode) {
		case MODE_2013_FB_FPGA:
			Firmware::fb_upload(hex, onboard, leave_powered);
			break;
	}

	return 0;
}

