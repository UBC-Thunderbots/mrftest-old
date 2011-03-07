#include "fw/ihex.h"
#include "util/annunciator.h"
#include "util/config.h"
#include "util/crc16.h"
#include "util/dprint.h"
#include "util/noncopyable.h"
#include "xbee/dongle.h"
#include <glibmm.h>
#include <iostream>
#include <locale>
#include <string>

namespace {
	const std::size_t CRC_BLOCK_SIZE = 16384;

	class FirmwareUploadOperation : public NonCopyable, public sigc::trackable {
		public:
			FirmwareUploadOperation(const Config &config, const IntelHex &hex, bool fpga, int robot, XBeeDongle &dongle, Glib::RefPtr<Glib::MainLoop> main_loop) : config(config), hex(hex), fpga(fpga), robot(robot), dongle(dongle), main_loop(main_loop) {
				Glib::signal_idle().connect_once(sigc::mem_fun(this, &FirmwareUploadOperation::start_operation));
			}

			~FirmwareUploadOperation() {
			}

		private:
			const Config &config;
			const IntelHex &hex;
			const bool fpga;
			const int robot;
			XBeeDongle &dongle;
			const Glib::RefPtr<Glib::MainLoop> main_loop;
			sigc::connection alive_changed_connection;
			unsigned int byte, page, block;

			void start_operation() {
				std::cout << "Enabling radios... " << std::flush;
				dongle.xbees_state.signal_changed().connect(sigc::mem_fun(this, &FirmwareUploadOperation::on_xbees_state_changed));
				AsyncOperation<void>::Ptr op = dongle.enable();
				op->signal_done.connect(sigc::mem_fun(this, &FirmwareUploadOperation::on_radios_enabled));
			}

			void on_xbees_state_changed() {
				switch (dongle.xbees_state) {
					case XBeeDongle::XBEES_STATE_PREINIT:
					case XBeeDongle::XBEES_STATE_INIT1_0:
					case XBeeDongle::XBEES_STATE_INIT1_1:
					case XBeeDongle::XBEES_STATE_INIT1_DONE:
					case XBeeDongle::XBEES_STATE_INIT2_0:
					case XBeeDongle::XBEES_STATE_INIT2_1:
					case XBeeDongle::XBEES_STATE_RUNNING:
						return;

					case XBeeDongle::XBEES_STATE_FAIL_0:
						throw std::runtime_error("XBee 0 initialization failed");

					case XBeeDongle::XBEES_STATE_FAIL_1:
						throw std::runtime_error("XBee 1 initialization failed");
				}

				throw std::runtime_error("XBees in unknown state");
			}

			void on_radios_enabled(AsyncOperation<void>::Ptr op) {
				op->result();
				std::cout << "OK\n";
				std::cout << "Waiting for robot to appear... " << std::flush;
				alive_changed_connection = dongle.robot(robot)->alive.signal_changed().connect(sigc::mem_fun(this, &FirmwareUploadOperation::on_alive_changed));
				on_alive_changed();
			}

			void on_alive_changed() {
				if (dongle.robot(robot)->alive) {
					alive_changed_connection.disconnect();
					alive_changed_connection = dongle.robot(robot)->alive.signal_changed().connect(sigc::mem_fun(this, &FirmwareUploadOperation::on_alive_changed2));
					std::cout << "OK\n";
					std::cout << "Erasing flash... " << std::flush;
					dongle.robot(robot)->firmware_spi_chip_erase()->signal_done.connect(sigc::mem_fun(this, &FirmwareUploadOperation::on_spi_chip_erase_done));
				}
			}

			void on_alive_changed2() {
				if (!dongle.robot(robot)->alive) {
					std::cout << "Robot unexpectedly died\n";
					main_loop->quit();
				}
			}

			void on_spi_chip_erase_done(AsyncOperation<void>::Ptr op) {
				op->result();
				std::cout << "OK\n";
				byte = 0;
				page = 0;
				start_fill_page_buffer();
			}

			void start_fill_page_buffer() {
				if (!byte) {
					std::cout << "\rWriting page " << (page + 1) << '/' << ((hex.data()[0].size() + 255) / 256) << "... " << std::flush;
				}
				uint8_t stage[61];
				std::size_t to_fill = sizeof(stage);
				if (byte + to_fill > 256) {
					to_fill = 256 - byte;
				}
				for (std::size_t i = 0; i < to_fill; ++i) {
					if (page * 256 + byte + i < hex.data()[0].size()) {
						stage[i] = hex.data()[0][page * 256 + byte + i];
					} else {
						stage[i] = 0xFF;
					}
				}
				dongle.robot(robot)->firmware_spi_fill_page_buffer(byte, stage, to_fill)->signal_done.connect(sigc::bind(sigc::mem_fun(this, &FirmwareUploadOperation::on_fill_page_buffer_done), to_fill));
			}

			void on_fill_page_buffer_done(AsyncOperation<void>::Ptr op, std::size_t to_fill) {
				op->result();
				byte = static_cast<unsigned int>(byte + to_fill);
				if (byte < 256) {
					start_fill_page_buffer();
				} else {
					start_page_program();
				}
			}

			void start_page_program() {
				uint8_t stage[256];
				for (std::size_t i = 0; i < sizeof(stage); ++i) {
					if (page * 256 + i < hex.data()[0].size()) {
						stage[i] = hex.data()[0][page * 256 + i];
					} else {
						stage[i] = 0xFF;
					}
				}
				uint16_t crc = CRC16::calculate(stage, sizeof(stage));
				dongle.robot(robot)->firmware_spi_page_program(page, crc)->signal_done.connect(sigc::mem_fun(this, &FirmwareUploadOperation::on_page_program_done));
			}

			void on_page_program_done(AsyncOperation<void>::Ptr op) {
				op->result();
				++page;
				if (page < (hex.data()[0].size() + 255) / 256) {
					byte = 0;
					start_fill_page_buffer();
				} else {
					std::cout << "OK\n";
					block = 0;
					start_block_crc();
				}
			}

			void start_block_crc() {
				std::cout << "CRC-checking block " << (block + 1) << '/' << ((hex.data()[0].size() + CRC_BLOCK_SIZE - 1) / CRC_BLOCK_SIZE) << "... " << std::flush;
				dongle.robot(robot)->firmware_spi_block_crc(static_cast<unsigned int>(block * CRC_BLOCK_SIZE), std::min(CRC_BLOCK_SIZE, hex.data()[0].size() - block * CRC_BLOCK_SIZE))->signal_done.connect(sigc::mem_fun(this, &FirmwareUploadOperation::on_block_crc_done));
			}

			void on_block_crc_done(AsyncOperation<uint16_t>::Ptr op) {
				uint16_t crc = op->result();
				uint16_t calculated = CRC16::calculate(&hex.data()[0][block * CRC_BLOCK_SIZE], std::min(CRC_BLOCK_SIZE, hex.data()[0].size() - block * CRC_BLOCK_SIZE));
				if (crc != calculated) {
					std::cout << "Failed, expected " << calculated << ", got " << crc << '\n';
					main_loop->quit();
					return;
				}
				++block;
				if (block < (hex.data()[0].size() + CRC_BLOCK_SIZE - 1) / CRC_BLOCK_SIZE) {
					std::cout << '\r';
					start_block_crc();
				} else {
					std::cout << "OK\n";
					std::cout << "Reading operational parameters block... " << std::flush;
					dongle.robot(robot)->firmware_read_operational_parameters()->signal_done.connect(sigc::mem_fun(this, &FirmwareUploadOperation::on_read_operational_parameters_done));
				}
			}

			void on_read_operational_parameters_done(AsyncOperation<XBeeRobot::OperationalParameters>::Ptr op) {
				XBeeRobot::OperationalParameters params = op->result();
				std::cout << "OK\n";
				params.flash_contents = fpga ? XBeeRobot::OperationalParameters::FlashContents::FLASH_CONTENTS_FPGA : XBeeRobot::OperationalParameters::FlashContents::FLASH_CONTENTS_PIC;
				std::cout << "Setting operational parameters block... " << std::flush;
				dongle.robot(robot)->firmware_set_operational_parameters(params)->signal_done.connect(sigc::mem_fun(this, &FirmwareUploadOperation::on_set_operational_parameters_done));
			}

			void on_set_operational_parameters_done(AsyncOperation<void>::Ptr op) {
				op->result();
				std::cout << "OK\n";
				std::cout << "Committing operational parameters block... " << std::flush;
				dongle.robot(robot)->firmware_commit_operational_parameters()->signal_done.connect(sigc::mem_fun(this, &FirmwareUploadOperation::on_commit_operational_parameters_done));
			}

			void on_commit_operational_parameters_done(AsyncOperation<void>::Ptr op) {
				op->result();
				std::cout << "OK\n";
				std::cout << "Rebooting microcontroller... " << std::flush;
				dongle.robot(robot)->firmware_reboot()->signal_done.connect(sigc::mem_fun(this, &FirmwareUploadOperation::on_reboot_done));
			}

			void on_reboot_done(AsyncOperation<void>::Ptr op) {
				op->result();
				std::cout << "OK\n";
				main_loop->quit();
			}
	};

	int main_impl(int argc, char **argv) {
		// Set the current locale from environment variables.
		std::locale::global(std::locale(""));

		// Parse the command-line arguments.
		Glib::OptionContext option_context;
		option_context.set_summary("Updates the firmware on a robot.");

		Glib::OptionGroup option_group("thunderbots", "Firmware Updater Options", "Show Firmware Updater Options");

		Glib::OptionEntry fpga_option;
		fpga_option.set_long_name("fpga");
		fpga_option.set_short_name('f');
		fpga_option.set_description("Updates the FPGA's bitstream");
		bool fpga = false;
		option_group.add_entry(fpga_option, fpga);

		Glib::OptionEntry pic_option;
		pic_option.set_long_name("pic");
		pic_option.set_short_name('p');
		pic_option.set_description("Updates the PIC's firmware");
		bool pic = false;
		option_group.add_entry(pic_option, pic);

		Glib::OptionEntry robot_option;
		robot_option.set_long_name("robot");
		robot_option.set_short_name('r');
		robot_option.set_description("Selects the robot to update");
		robot_option.set_arg_description("ROBOT");
		int robot = 0;
		option_group.add_entry(robot_option, robot);

		Glib::OptionEntry signature_option;
		signature_option.set_long_name("signature");
		signature_option.set_short_name('s');
		signature_option.set_description("Displays the build signature of the hex file");
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

		if ((!fpga && !pic) || (fpga && pic)) {
			std::cerr << "Exactly one of --fpga and --pic must be specified.\n";
			return 1;
		}
		if (!(1 <= robot && robot <= 15) && !signature) {
			std::cerr << "--robot must be between 1 and 15 or --signature must be specified.\n";
			return 1;
		}
		if (!hex_filename.size()) {
			std::cerr << "--hex must be specified.\n";
			return 1;
		}

		IntelHex hex;
		if (pic) {
			hex.add_section(0, 128 * 1024);
		} else if (fpga) {
			hex.add_section(0, 2 * 1024 * 1024);
		}
		hex.load(hex_filename);

		if (signature) {
			const std::vector<unsigned char> &v = hex.data()[0];
			uint16_t crc;
			if (pic) {
				crc = CRC16::calculate(&v[0], std::min(v.size(), static_cast<std::size_t>(0x1F000)));
				for (std::size_t i = v.size(); i < 0x1F000; ++i) {
					crc = CRC16::calculate(0xFF, crc);
				}
			} else {
				crc = CRC16::calculate(&v[0], v.size());
				for (std::size_t i = v.size(); i < 2 * 1024 * 1024; ++i) {
					crc = CRC16::calculate(0xFF, crc);
				}
			}
			std::cout << "Build signature is 0x" << tohex(crc, 4) << '\n';
		}

		if (1 <= robot && robot <= 15) {
			Config config;

			std::cout << "Finding dongle... " << std::flush;
			XBeeDongle dongle(config.out_channel(), config.in_channel());
			std::cout << "OK\n";

			Glib::RefPtr<Glib::MainLoop> main_loop = Glib::MainLoop::create();
			FirmwareUploadOperation op(config, hex, fpga, robot, dongle, main_loop);
			main_loop->run();
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

