#include "fw/xbee.h"
#include "util/async_operation.h"
#include "util/crc16.h"
#include "util/noncopyable.h"
#include "xbee/dongle.h"
#include "xbee/robot.h"
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <memory>
#include <stdint.h>
#include <glibmm/main.h>
#include <glibmm/refptr.h>
#include <sigc++/connection.h>
#include <sigc++/trackable.h>
#include <sigc++/functors/mem_fun.h>

namespace {
	const std::size_t CRC_BLOCK_SIZE = 16384;

	class FirmwareUploadOperation : public NonCopyable, public sigc::trackable {
		public:
			FirmwareUploadOperation(const Firmware::IntelHex &hex, bool fpga, int robot, XBeeDongle &dongle, Glib::RefPtr<Glib::MainLoop> main_loop) : hex(hex), fpga(fpga), robot(robot), dongle(dongle), main_loop(main_loop) {
				Glib::signal_idle().connect_once(sigc::mem_fun(this, &FirmwareUploadOperation::start_operation));
			}

		private:
			const Firmware::IntelHex &hex;
			const bool fpga;
			const int robot;
			XBeeDongle &dongle;
			const Glib::RefPtr<Glib::MainLoop> main_loop;
			sigc::connection alive_changed_connection;
			std::unique_ptr<AsyncOperation<void>> void_op;
			std::unique_ptr<AsyncOperation<uint16_t>> crc_op;
			std::unique_ptr<AsyncOperation<XBeeRobot::OperationalParameters>> read_opparams_op;
			unsigned int byte, page, block;

			void start_operation() {
				std::cout << "Waiting for robot to appear... " << std::flush;
				alive_changed_connection = dongle.robot(robot).alive.signal_changed().connect(sigc::mem_fun(this, &FirmwareUploadOperation::on_alive_changed));
				on_alive_changed();
			}

			void on_alive_changed() {
				if (dongle.robot(robot).alive) {
					alive_changed_connection.disconnect();
					alive_changed_connection = dongle.robot(robot).alive.signal_changed().connect(sigc::mem_fun(this, &FirmwareUploadOperation::on_alive_changed2));
					std::cout << "OK\n";
					std::cout << "Erasing flash... " << std::flush;
					void_op.reset(new XBeeRobot::FirmwareSPIChipEraseOperation(dongle.robot(robot)));
					void_op->signal_done.connect(sigc::mem_fun(this, &FirmwareUploadOperation::on_spi_chip_erase_done));
				}
			}

			void on_alive_changed2() {
				if (!dongle.robot(robot).alive) {
					std::cout << "Robot unexpectedly died\n";
					main_loop->quit();
				}
			}

			void on_spi_chip_erase_done(AsyncOperation<void> &op) {
				op.result();
				std::cout << "OK\n";
				byte = 0;
				page = 0;
				start_fill_page_buffer();
			}

			void start_fill_page_buffer() {
				if (!byte) {
					std::cout << "\rWriting page " << (page + 1) << '/' << ((hex.data()[0].size() + 255) / 256) << "... " << std::flush;
				}
				unsigned char stage[61];
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
				void_op.reset(new XBeeRobot::FirmwareSPIFillPageBufferOperation(dongle.robot(robot), byte, stage, to_fill));
				void_op->signal_done.connect(sigc::bind(sigc::mem_fun(this, &FirmwareUploadOperation::on_fill_page_buffer_done), to_fill));
			}

			void on_fill_page_buffer_done(AsyncOperation<void> &op, std::size_t to_fill) {
				op.result();
				byte = static_cast<unsigned int>(byte + to_fill);
				if (byte < 256) {
					start_fill_page_buffer();
				} else {
					start_page_program();
				}
			}

			void start_page_program() {
				unsigned char stage[256];
				for (std::size_t i = 0; i < sizeof(stage); ++i) {
					if (page * 256 + i < hex.data()[0].size()) {
						stage[i] = hex.data()[0][page * 256 + i];
					} else {
						stage[i] = 0xFF;
					}
				}
				uint16_t crc = CRC16::calculate(stage, sizeof(stage));
				void_op.reset(new XBeeRobot::FirmwareSPIPageProgramOperation(dongle.robot(robot), page, crc));
				void_op->signal_done.connect(sigc::mem_fun(this, &FirmwareUploadOperation::on_page_program_done));
			}

			void on_page_program_done(AsyncOperation<void> &op) {
				op.result();
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
				crc_op.reset(new XBeeRobot::FirmwareSPIBlockCRCOperation(dongle.robot(robot), static_cast<unsigned int>(block * CRC_BLOCK_SIZE), std::min(CRC_BLOCK_SIZE, hex.data()[0].size() - block * CRC_BLOCK_SIZE)));
				crc_op->signal_done.connect(sigc::mem_fun(this, &FirmwareUploadOperation::on_block_crc_done));
			}

			void on_block_crc_done(AsyncOperation<uint16_t> &op) {
				uint16_t crc = op.result();
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
					read_opparams_op.reset(new XBeeRobot::FirmwareReadOperationalParametersOperation(dongle.robot(robot)));
					read_opparams_op->signal_done.connect(sigc::mem_fun(this, &FirmwareUploadOperation::on_read_operational_parameters_done));
				}
			}

			void on_read_operational_parameters_done(AsyncOperation<XBeeRobot::OperationalParameters> &op) {
				XBeeRobot::OperationalParameters params = op.result();
				std::cout << "OK\n";
				params.flash_contents = fpga ? XBeeRobot::OperationalParameters::FlashContents::FPGA : XBeeRobot::OperationalParameters::FlashContents::PIC;
				std::cout << "Setting operational parameters block... " << std::flush;
				void_op.reset(new XBeeRobot::FirmwareWriteOperationalParametersOperation(dongle.robot(robot), params));
				void_op->signal_done.connect(sigc::mem_fun(this, &FirmwareUploadOperation::on_set_operational_parameters_done));
			}

			void on_set_operational_parameters_done(AsyncOperation<void> &op) {
				op.result();
				std::cout << "OK\n";
				std::cout << "Committing operational parameters block... " << std::flush;
				void_op.reset(new XBeeRobot::FirmwareCommitOperationalParametersOperation(dongle.robot(robot)));
				void_op->signal_done.connect(sigc::mem_fun(this, &FirmwareUploadOperation::on_commit_operational_parameters_done));
			}

			void on_commit_operational_parameters_done(AsyncOperation<void> &op) {
				op.result();
				std::cout << "OK\n";
				std::cout << "Rebooting microcontroller... " << std::flush;
				void_op.reset(new XBeeRobot::RebootOperation(dongle.robot(robot)));
				void_op->signal_done.connect(sigc::mem_fun(this, &FirmwareUploadOperation::on_reboot_done));
			}

			void on_reboot_done(AsyncOperation<void> &op) {
				op.result();
				std::cout << "OK\n";
				main_loop->quit();
			}
	};
}

void Firmware::xbee_upload(const IntelHex &hex, bool fpga, int robot) {
	std::cout << "Finding dongle... " << std::flush;
	XBeeDongle dongle;
	std::cout << "OK\n";

	std::cout << "Enabling radios... " << std::flush;
	dongle.enable();
	std::cout << "OK\n";

	Glib::RefPtr<Glib::MainLoop> main_loop = Glib::MainLoop::create();
	FirmwareUploadOperation op(hex, fpga, robot, dongle, main_loop);
	main_loop->run();
}

