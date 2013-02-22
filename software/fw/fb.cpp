#include "fw/fb.h"
#include "util/libusb.h"
#include "util/string.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <glibmm/convert.h>
#include <glibmm/timer.h>

#define PAGE_SIZE 256
#define READ_BLOCK_SIZE 4096

namespace {
	enum {
		CONTROL_REQUEST_READ_IO,
		CONTROL_REQUEST_WRITE_IO,
		CONTROL_REQUEST_JEDEC_ID,
		CONTROL_REQUEST_READ_STATUS,
		CONTROL_REQUEST_READ,
		CONTROL_REQUEST_WRITE,
		CONTROL_REQUEST_ERASE,
	};

	enum {
		PIN_MOSI = 1,
		PIN_MISO = 2,
		PIN_CLOCK = 4,
		PIN_CS = 8,
		PIN_POWER = 16,
		PIN_PROGRAM_B = 32,
	};

	void write_io(USB::DeviceHandle &dev, uint8_t levels, uint8_t directions) {
		dev.control_no_data(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, CONTROL_REQUEST_WRITE_IO, static_cast<uint16_t>((directions << 8) | levels), 0, 250);
	}

	uint32_t read_jedec_id(USB::DeviceHandle &dev) {
		uint8_t jedec[3];
		dev.control_in(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, CONTROL_REQUEST_JEDEC_ID, 0, 0, jedec, sizeof(jedec), 250);
		return (jedec[0] << 16) | (jedec[1] << 8) | jedec[2];
	}

	uint8_t read_status_register(USB::DeviceHandle &dev) {
		uint8_t status;
		dev.control_in(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, CONTROL_REQUEST_READ_STATUS, 0, 0, &status, sizeof(status), 250);
		return status;
	}

	void erase_chip(USB::DeviceHandle &dev) {
		dev.control_no_data(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, CONTROL_REQUEST_ERASE, 0, 0, 250);
		uint8_t byte;
		dev.interrupt_in(1, &byte, sizeof(byte), 15000);
	}

	void write_page(USB::DeviceHandle &dev, uint16_t page, const void *data) {
		dev.control_out(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, CONTROL_REQUEST_WRITE, page, 0, data, PAGE_SIZE, 250);
		uint8_t byte;
		dev.interrupt_in(1, &byte, sizeof(byte), 250);
	}

	void read_data(USB::DeviceHandle &dev, uint16_t page, void *data, std::size_t length) {
		std::size_t sz = dev.control_in(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_INTERFACE, CONTROL_REQUEST_READ, page, 0, data, length, 5000);
		if (sz != length) { std::cout << "Asked for a read of " << length << " bytes but got " << sz << "!\n"; }
	}
}

void Firmware::fb_upload(const IntelHex &hex, bool leave_powered) {
	// Find and open the burner module.
	std::cout << "Finding and activating burner module… ";
	std::cout.flush();
	USB::Context context;
	USB::DeviceHandle handle(context, 0x0483, 0x497D);
	USB::ConfigurationSetter config_setter(handle, 2);
	USB::InterfaceClaimer interface_claimer(handle, 0);
	handle.set_interface_alt_setting(0, 1);
	std::cout << "OK.\n";

	// Drive PROGRAM_B low and power control high, let the board stabilize, and drive the SPI bus.
	std::cout << "Activating target board… ";
	std::cout.flush();
	write_io(handle, PIN_POWER, PIN_POWER | PIN_PROGRAM_B);
	Glib::usleep(1000);
	write_io(handle, PIN_CS | PIN_POWER, PIN_MOSI | PIN_CLOCK | PIN_CS | PIN_POWER | PIN_PROGRAM_B);
	Glib::usleep(1000);
	std::cout << "OK.\n";

	// Read the JEDEC ID.
	std::cout << "Reading JEDEC ID… ";
	std::cout.flush();
	uint32_t jedec = read_jedec_id(handle);
	std::cout << Glib::locale_from_utf8(tohex(jedec, 6));
	if (jedec != 0xEF4015) {
		std::cout << " (unknown)!\n";
		return;
	} else {
		std::cout << " (OK).\n";
	}

	// Read the status register.
	std::cout << "Reading status register… ";
	std::cout.flush();
	uint8_t status = read_status_register(handle);
	std::cout << Glib::locale_from_utf8(tohex(status, 2));
	if (status & 1) {
		std::cout << " (already busy with an operation)!\n";
		return;
	} else if (status & 0b11100) {
		std::cout << " (write protected)!\n";
		return;
	} else {
		std::cout << " (OK).\n";
	}

	// Erase the chip.
	std::cout << "Erasing chip… ";
	std::cout.flush();
	erase_chip(handle);
	std::cout << "OK.\n";

	// Write the data.
	std::cout << "Writing data… ";
	std::cout.flush();
	for (std::size_t i = 0; i < hex.data()[0].size(); i += PAGE_SIZE) {
		if (i + PAGE_SIZE <= hex.data()[0].size()) {
			write_page(handle, static_cast<uint16_t>(i / PAGE_SIZE), &hex.data()[0][i]);
		} else {
			std::array<uint8_t, PAGE_SIZE> stage;
			stage.fill(0xFF);
			std::copy(hex.data()[0].begin() + i, hex.data()[0].end(), stage.begin());
			write_page(handle, static_cast<uint16_t>(i / PAGE_SIZE), &stage[0]);
		}
		std::cout << "\rWriting data… " << std::min(i + PAGE_SIZE, hex.data()[0].size()) << '/' << hex.data()[0].size();
		std::cout.flush();
	}
	std::cout << "\rWriting data… OK.                \n";

	// Read back and compare the data.
	std::cout << "Reading and comparing data… ";
	std::cout.flush();
	for (std::size_t i = 0; i < hex.data()[0].size(); i += READ_BLOCK_SIZE) {
		std::array<uint8_t, READ_BLOCK_SIZE> buffer;
		read_data(handle, static_cast<uint16_t>(i / PAGE_SIZE), &buffer[0], buffer.size());
		for (std::size_t j = 0; j < buffer.size() && i + j < hex.data()[0].size(); ++j) {
			if (buffer[j] != hex.data()[0][i + j]) {
				std::cout << "\rReading and comparing data… compare failed at byte " << (i + j) << ": expected " << Glib::locale_from_utf8(tohex(hex.data()[0][i + j], 2)) << " but found " << Glib::locale_from_utf8(tohex(buffer[j], 2)) << '\n';
				return;
			}
		}
		std::cout << "\rReading and comparing data… " << std::min(i + READ_BLOCK_SIZE, hex.data()[0].size()) << '/' << hex.data()[0].size();
	}
	std::cout << "\rReading and comparing data… OK.               \n";

	// Release the bus.
	if (leave_powered) {
		// Float PROGRAM_B, wait a while, and then let go of power control—by that time, the FPGA should be holding itself up.
		std::cout << "Allowing FPGA to boot… ";
		std::cout.flush();
		write_io(handle, PIN_POWER, PIN_POWER);
		Glib::usleep(500000);
		write_io(handle, 0, 0);
		std::cout << "OK.\n";
	} else {
		// Drive power control low while holding PROGRAM_B low, wait a while, and then let go of the bus once it’s certain everything is dead.
		std::cout << "Powering down board… ";
		std::cout.flush();
		write_io(handle, 0, PIN_POWER | PIN_PROGRAM_B);
		Glib::usleep(300000);
		write_io(handle, 0, 0);
		std::cout << "OK.\n";
	}

	// Show a final message.
	std::cout << "Operation complete.\n";
}

