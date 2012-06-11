#include "fw/pk2.h"
#include "util/libusb.h"
#include "util/string.h"
#include <algorithm>
#include <alloca.h>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <glibmm/convert.h>
#include <glibmm/ustring.h>

namespace {
	enum PK2_CMDS {
		ENTER_BOOTLOADER = 0x42,
		NO_OPERATION = 0x5A,
		FIRMWARE_VERSION = 0x76,
		SETVDD = 0xA0,
		SETVPP,
		READ_STATUS,
		READ_VOLTAGES,
		DOWNLOAD_SCRIPT,
		RUN_SCRIPT,
		EXECUTE_SCRIPT,
		CLR_DOWNLOAD_BUFFER,
		DOWNLOAD_DATA,
		CLR_UPLOAD_BUFFER,
		UPLOAD_DATA,
		CLR_SCRIPT_BUFFER,
		UPLOAD_DATA_NOLEN,
		END_OF_BUFFER,
		RESET,
		SCRIPT_BUFFER_CHKSUM,
		SET_VOLTAGE_CALS,
		WR_INTERNAL_EE,
		RD_INTERNAL_EE,
		ENTER_UART_MODE,
		EXIT_UART_MODE,
		ENTER_LEARN_MODE,
		EXIT_LEARN_MODE,
		ENABLE_PK2GO_MODE,
		LOGIC_ANALYZER_GO,
		COPY_RAM_UPLOAD,
		READ_OSCCAL = 0x80,
		WRITE_OSCCAL,
		START_CHECKSUM,
		VERIFY_CHECKSUM,
		CHECK_DEVICE_ID,
		READ_BANDGAP,
		WRITE_CFG_BANDGAP,
		CHANGE_CHKSM_FRMT
	};

	enum PK2_SCRIPT_CMDS {
		JT2_PE_PROG_RESP = 0xB3,
		JT2_WAIT_PE_RESP,
		JT2_GET_PE_RESP,
		JT2_XFERINST_BUF,
		JT2_XFRFASTDAT_BUF,
		JT2_XFRFASTDAT_LIT,
		JT2_XFERDATA32_LIT,
		JT2_XFERDATA8_LIT,
		JT2_SENDCMD,
		JT2_SETMODE,
		UNIO_TX_RX,
		UNIO_TX,
		MEASURE_PULSE,
		ICDSLAVE_TX_BUF_BL,
		ICDSLAVE_TX_LIT_BL,
		ICDSLAVE_RX_BL,
		SPI_RDWR_BYTE_BUF,
		SPI_RDWR_BYTE_LIT,
		SPI_RD_BYTE_BUF,
		SPI_WR_BYTE_BUF,
		SPI_WR_BYTE_LIT,
		I2C_RD_BYTE_NACK,
		I2C_RD_BYTE_ACK,
		I2C_WR_BYTE_BUF,
		I2C_WR_BYTE_LIT,
		I2C_STOP,
		I2C_START,
		AUX_STATE_BUFFER,
		SET_AUX,
		WRITE_BITS_BUF_HLD,
		WRITE_BITS_LIT_HLD,
		CONST_WRITE_DL,
		WRITE_BUFBYTE_W,
		WRITE_BUFWORD_W,
		RD2_BITS_BUFFER,
		RD2_BYTE_BUFFER,
		VISI24,
		NOP24,
		COREINST24,
		COREINST18,
		POP_DOWNLOAD,
		ICSP_STATES_BUFFER,
		LOOPBUFFER,
		ICDSLAVE_TX_BUF,
		ICDSLAVE_TX_LIT,
		ICDSLAVE_RX,
		POKE_SFR,
		PEEK_SFR,
		EXIT_SCRIPT,
		GOTO_INDEX,
		IF_GT_GOTO,
		IF_EQ_GOTO,
		DELAY_SHORT,
		DELAY_LONG,
		LOOP,
		SET_ICSP_SPEED,
		READ_BITS,
		READ_BITS_BUFFER,
		WRITE_BITS_BUFFER,
		WRITE_BITS_LITERAL,
		READ_BYTE,
		READ_BYTE_BUFFER,
		WRITE_BYTE_BUFFER,
		WRITE_BYTE_LITERAL,
		SET_ICSP_PINS,
		BUSY_LED_OFF,
		BUSY_LED_ON,
		MCLR_GND_OFF,
		MCLR_GND_ON,
		VPP_PWM_OFF,
		VPP_PWM_ON,
		VPP_OFF,
		VPP_ON,
		VDD_GND_OFF,
		VDD_GND_ON,
		VDD_OFF,
		VDD_ON
	};

	void pk2_write(USB::DeviceHandle &handle, const void *data, std::size_t length) {
		assert(length <= 64);
		if (length < 64) {
			unsigned char *buffer = static_cast<unsigned char *>(alloca(64));
			if (data) {
				std::memcpy(buffer, data, length);
			}
			std::memset(buffer + length, END_OF_BUFFER, 64 - length);
			data = buffer;
			length = 64;
		}
		handle.interrupt_out(1, data, length, 5000);
	}

	void pk2_read(USB::DeviceHandle &handle, void *dest, std::size_t length) {
		void *buffer = length >= 64 ? dest : alloca(64);
		handle.interrupt_in(1, buffer, 64, 5000);
		if (dest && buffer != dest) {
			std::memcpy(dest, buffer, std::min<std::size_t>(length, 64));
		}
	}

	void check_status(USB::DeviceHandle &handle) {
		static const unsigned char COMMAND = READ_STATUS;
		pk2_write(handle, &COMMAND, sizeof(COMMAND));
		unsigned char buffer[64];
		pk2_read(handle, buffer, sizeof(buffer));
		std::string errors;
		if (buffer[0] & (1 << 5)) {
			if (!errors.empty()) {
				errors += ", ";
			}
			errors += "VPP fault";
		}
		if (buffer[0] & (1 << 4)) {
			if (!errors.empty()) {
				errors += ", ";
			}
			errors += "VDD fault";
		}
		if (buffer[1] & (1 << 7)) {
			if (!errors.empty()) {
				errors += ", ";
			}
			errors += "download buffer overflow";
		}
		if (buffer[1] & (1 << 6)) {
			if (!errors.empty()) {
				errors += ", ";
			}
			errors += "script buffer overflow, invalid script length, or invalid script index";
		}
		if (buffer[1] & (1 << 5)) {
			if (!errors.empty()) {
				errors += ", ";
			}
			errors += "attempt to run empty script";
		}
		if (buffer[1] & (1 << 4)) {
			if (!errors.empty()) {
				errors += ", ";
			}
			errors += "download buffer underflow";
		}
		if (buffer[1] & (1 << 3)) {
			if (!errors.empty()) {
				errors += ", ";
			}
			errors += "upload buffer overflow";
		}
		if (buffer[1] & (1 << 2)) {
			if (!errors.empty()) {
				errors += ", ";
			}
			errors += "ICD transfer timeout/bus error";
		}
		if (buffer[1] & (1 << 1)) {
			if (!errors.empty()) {
				errors += ", ";
			}
			errors += "UART mode enabled";
		}
		if (buffer[1] & (1 << 0)) {
			if (!errors.empty()) {
				errors += ", ";
			}
			errors += "device reset";
		}
		if (!errors.empty()) {
			throw std::runtime_error("PICkit2 reported error(s): " + errors);
		}
	}

	void expect_length_header(const unsigned char *buffer, unsigned char expected) {
		if (buffer[0] != expected) {
			throw std::runtime_error(Glib::locale_from_utf8(Glib::ustring::compose("PICkit2 returned upload buffer with %1 bytes, expected %2.", buffer[0] + 0U, expected + 0U)));;
		}
	}
}

void Firmware::pk2_upload(const IntelHex &hex) {
	// Find and open the PICKit2
	USB::Context context;
	USB::DeviceHandle handle(context, 0x04D8, 0x0033);
	if (handle.get_configuration() != 2) {
		handle.set_configuration(2);
	}
	handle.claim_interface(0);

	// Check PICkit2 firmware version
	{
		std::cout << "Checking PICkit2 firmware...\n";
		const unsigned char command[] = { FIRMWARE_VERSION };
		pk2_write(handle, command, sizeof(command));
		unsigned char response[3];
		pk2_read(handle, response, sizeof(response));
		if (response[0] != 2 || response[1] < 32) {
			std::cout << "PICkit2 firmware version " << (response[0] + 0U) << '.' << (response[1] + 0U) << '.' << (response[2] + 0U) << " is not verified for use with this tool.\n";
			std::cout << "Only major version 2 with minor version at least 32 is verified.\n";
			return;
		}
	}

	// Read PICkit2 status, clean up any strange state, and initialize the pins
	{
		std::cout << "Checking PICkit2 status...\n";
		const unsigned char command = READ_STATUS;
		pk2_write(handle, &command, sizeof(command));
		unsigned char response[2];
		pk2_read(handle, response, sizeof(response));
		std::vector<unsigned char> cleanup_commands;
		if (response[1] & (1 << 1)) {
			std::cout << "WARNING: PICkit2 was in UART mode; exiting.\n";
			cleanup_commands.push_back(EXIT_UART_MODE);
		}
		if (response[0] & ((1 << 1) | (1 << 0))) {
			std::cout << "WARNING: PICkit2 VDD driver was enabled; disabling.\n";
			cleanup_commands.push_back(EXECUTE_SCRIPT);
			cleanup_commands.push_back(2);
			cleanup_commands.push_back(VDD_OFF);
			cleanup_commands.push_back(VDD_GND_OFF);
		}
		cleanup_commands.push_back(CLR_DOWNLOAD_BUFFER);
		cleanup_commands.push_back(CLR_UPLOAD_BUFFER);
		cleanup_commands.push_back(CLR_SCRIPT_BUFFER);
		cleanup_commands.push_back(EXECUTE_SCRIPT);
		cleanup_commands.push_back(9);
		cleanup_commands.push_back(VPP_PWM_OFF);
		cleanup_commands.push_back(MCLR_GND_OFF);
		cleanup_commands.push_back(VPP_ON);
		cleanup_commands.push_back(SET_ICSP_PINS);
		cleanup_commands.push_back(1 << 1); // PGD = MISO = input, PGC = CLK = output 0
		cleanup_commands.push_back(SET_AUX);
		cleanup_commands.push_back(0); // AUX = MOSI = output 0
		cleanup_commands.push_back(SET_ICSP_SPEED);
		cleanup_commands.push_back(1); // Clock at 1 MHz
		pk2_write(handle, &cleanup_commands[0], cleanup_commands.size());
		check_status(handle);
	}

	// Read the JEDEC ID from the SPI flash
	{
		std::cout << "Checking SPI flash manufacturer and model ID...\n";
		const unsigned char command[] = {
			EXECUTE_SCRIPT, 11,
				VPP_OFF,
				BUSY_LED_ON,
				MCLR_GND_ON,
				SPI_WR_BYTE_LIT, 0x9F,
				SPI_RD_BYTE_BUF,
				SPI_RD_BYTE_BUF,
				SPI_RD_BYTE_BUF,
				MCLR_GND_OFF,
				BUSY_LED_OFF,
				VPP_ON,
			UPLOAD_DATA
		};
		pk2_write(handle, command, sizeof(command));
		unsigned char buffer[64];
		pk2_read(handle, buffer, sizeof(buffer));
		check_status(handle);
		expect_length_header(buffer, 3);
		if (buffer[1] != 0xEF) {
			std::cerr << "ERROR: Read JEDEC ID returned manufacturer ID 0x" << tohex(buffer[1], 2) << ", expected 0xEF (Winbond).\n";
			return;
		}
		if (buffer[2] != 0x40 || buffer[3] != 0x15) {
			std::cerr << "ERROR: Read JEDEC ID returned memory type 0x" << tohex(buffer[2], 2) << tohex(buffer[3], 2) << ", expected 0x4015.\n";
			return;
		}
	}

	// Upload a handful of useful scripts
	enum {
		SCRIPT_INDEX_READ_STATUS_1,
		SCRIPT_INDEX_WRITE_ENABLE,
		SCRIPT_INDEX_PAGE_PROGRAM_BEGIN,
		SCRIPT_INDEX_PAGE_PROGRAM_END,
		SCRIPT_INDEX_READ_126_BYTES,
	};
	{
		std::cout << "Uploading scripts...\n";
		const unsigned char command[] = {
			DOWNLOAD_SCRIPT, SCRIPT_INDEX_READ_STATUS_1, 9,
				VPP_OFF,
				BUSY_LED_ON,
				MCLR_GND_ON,
				SPI_WR_BYTE_LIT, 0x05, // Read status register 1
				SPI_RD_BYTE_BUF,
				MCLR_GND_OFF,
				BUSY_LED_OFF,
				VPP_ON,
			DOWNLOAD_SCRIPT, SCRIPT_INDEX_WRITE_ENABLE, 8,
				VPP_OFF,
				BUSY_LED_ON,
				MCLR_GND_ON,
				SPI_WR_BYTE_LIT, 0x06,
				MCLR_GND_OFF,
				BUSY_LED_OFF,
				VPP_ON,
			DOWNLOAD_SCRIPT, SCRIPT_INDEX_PAGE_PROGRAM_BEGIN, 8,
				VPP_OFF,
				BUSY_LED_ON,
				MCLR_GND_ON,
				SPI_WR_BYTE_LIT, 0x02,
				SPI_WR_BYTE_BUF,
				SPI_WR_BYTE_BUF,
				SPI_WR_BYTE_BUF,
			DOWNLOAD_SCRIPT, SCRIPT_INDEX_PAGE_PROGRAM_END, 7,
				SPI_WR_BYTE_BUF,
				LOOP, 1, 255,
				MCLR_GND_OFF,
				BUSY_LED_OFF,
				VPP_ON,
			DOWNLOAD_SCRIPT, SCRIPT_INDEX_READ_126_BYTES, 15,
				VPP_OFF,
				BUSY_LED_ON,
				MCLR_GND_ON,
				SPI_WR_BYTE_LIT, 0x03,
				SPI_WR_BYTE_BUF,
				SPI_WR_BYTE_BUF,
				SPI_WR_BYTE_BUF,
				SPI_RD_BYTE_BUF,
				LOOP, 1, 125,
				MCLR_GND_OFF,
				BUSY_LED_OFF,
				VPP_ON,
		};
		pk2_write(handle, command, sizeof(command));
	}

	// Erase the chip
	{
		std::cout << "Erasing SPI flash...\n";
		{
			const unsigned char command[] = {
				RUN_SCRIPT, SCRIPT_INDEX_WRITE_ENABLE, 1,
				EXECUTE_SCRIPT, 8,
					VPP_OFF,
					BUSY_LED_ON,
					MCLR_GND_ON,
					SPI_WR_BYTE_LIT, 0xC7, // Chip erase
					MCLR_GND_OFF,
					BUSY_LED_OFF,
					VPP_ON,
			};
			pk2_write(handle, command, sizeof(command));
			check_status(handle);
		}
		bool done = false;
		while (!done) {
			const unsigned char command[] = {
				RUN_SCRIPT, SCRIPT_INDEX_READ_STATUS_1, 1,
				UPLOAD_DATA,
			};
			pk2_write(handle, command, sizeof(command));
			unsigned char buffer[64];
			pk2_read(handle, buffer, sizeof(buffer));
			check_status(handle);
			expect_length_header(buffer, 1);
			done = !(buffer[1] & 1);
		}
	}

	// Program the pages
	{
		for (std::size_t i = 0; i < 2 * 1024 * 1024 && i < hex.data()[0].size(); i += 256) {
			std::cout << "\rProgramming page " << (i / 256 + 1) << " of " << ((hex.data()[0].size() + 255) / 256) << "... ";
			std::cout.flush();
			{
				const unsigned char command[] = {
					DOWNLOAD_DATA, 3, static_cast<unsigned char>(i >> 16), static_cast<unsigned char>(i >> 8), 0,
					RUN_SCRIPT, SCRIPT_INDEX_WRITE_ENABLE, 1,
					RUN_SCRIPT, SCRIPT_INDEX_PAGE_PROGRAM_BEGIN, 1,
				};
				pk2_write(handle, command, sizeof(command));
				check_status(handle);
			}
			{
				unsigned char command[64];
				command[0] = DOWNLOAD_DATA;
				for (unsigned int j = 0; j < 256; j += 62) {
					command[1] = static_cast<unsigned char>(std::min(256U - j, 62U));
					std::size_t bytes_from_hex = i + j > hex.data()[0].size() ? 0 : std::min<std::size_t>(command[1], hex.data()[0].size() - (i + j));
					std::memcpy(command + 2, &hex.data()[0][i + j], bytes_from_hex);
					std::memset(command + 2 + bytes_from_hex, 0xFF, 62 - bytes_from_hex);
					pk2_write(handle, command, 2 + command[1]);
				}
				check_status(handle);
			}
			{
				const unsigned char command[] = {
					RUN_SCRIPT, SCRIPT_INDEX_PAGE_PROGRAM_END, 1
				};
				pk2_write(handle, command, sizeof(command));
				check_status(handle);
			}
			bool done = false;
			while (!done) {
				const unsigned char command[] = {
					RUN_SCRIPT, SCRIPT_INDEX_READ_STATUS_1, 1,
					UPLOAD_DATA,
				};
				pk2_write(handle, command, sizeof(command));
				unsigned char buffer[64];
				pk2_read(handle, buffer, sizeof(buffer));
				check_status(handle);
				expect_length_header(buffer, 1);
				done = !(buffer[1] & 1);
			}
		}
		std::cout << '\n';
	}

	// Verify the data
	{
		for (std::size_t i = 0; i < 2 * 1024 * 1024 && i < hex.data()[0].size(); i += 96) {
			std::cout << "\rVerifying page " << (i / 256 + 1) << " of " << ((hex.data()[0].size() + 255) / 256) << "... ";
			std::cout.flush();
			const unsigned char command[] = {
				DOWNLOAD_DATA, 3, static_cast<unsigned char>(i >> 16), static_cast<unsigned char>(i >> 8), static_cast<unsigned char>(i),
				RUN_SCRIPT, SCRIPT_INDEX_READ_126_BYTES, 1,
				UPLOAD_DATA,
				UPLOAD_DATA,
			};
			pk2_write(handle, command, sizeof(command));
			unsigned char buffer[2][64];
			pk2_read(handle, buffer[0], 64);
			pk2_read(handle, buffer[1], 64);
			check_status(handle);
			expect_length_header(buffer[0], 63);
			for (unsigned int j = 0; j < 126; ++j) {
				unsigned char readback = buffer[j / 63][(j % 63) + 1];
				unsigned char orig = (i + j) < hex.data()[0].size() ? hex.data()[0][i + j] : static_cast<unsigned char>(0xFF);
				if (readback != orig) {
					std::cout << "\nError verifying byte at address 0x" << tohex(i + j, 6) << ": expected 0x" << tohex(orig, 2) << ", readback 0x" << tohex(readback, 2) << ".\n";
					return;
				}
			}
		}
		std::cout << '\n';
	}

	// Tristate all I/O pins
	{
		static const unsigned char command[] = {
			EXECUTE_SCRIPT, 4,
			SET_ICSP_PINS, (1 << 1) | (1 << 0),
			SET_AUX, (1 << 0),
			EXECUTE_SCRIPT, 1,
				VPP_OFF,
		};
		pk2_write(handle, command, sizeof(command));
		check_status(handle);
	}

	// Show a final message
	std::cout << "Write OK.\n";
}

