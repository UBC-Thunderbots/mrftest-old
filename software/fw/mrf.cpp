#include "fw/mrf.h"
#include "fw/ihex.h"
#include "util/libusb.h"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <glibmm/convert.h>
#include <glibmm/ustring.h>

namespace {
	void check_mdr(USB::DeviceHandle &device, uint8_t message_id) {
		for (;;) {
			uint8_t mdr[2];
			std::size_t len = device.interrupt_in(1, mdr, sizeof(mdr), 0);
			if (len == sizeof(mdr)) {
				if (mdr[0] == message_id) {
					switch (mdr[1]) {
						case 0x00: return;
						case 0x01: throw std::runtime_error("Robot not associated!");
						case 0x02: throw std::runtime_error("Not acknowledged!");
						case 0x03: throw std::runtime_error("Clear channel error!");
						default: throw std::runtime_error("Unknown message delivery status!");
					}
				}
			} else if (len != 0) {
				throw std::runtime_error("Bad USB transfer size!");
			}
		}
	}
}

void Firmware::mrf_upload(const IntelHex &hex, unsigned int robot) {
	std::cout << "Opening dongle… ";
	std::cout.flush();
	USB::Context context;
	USB::DeviceHandle device(context, 0xC057, 0x2579, "303730303331470100240033");
	std::cout << "OK\n";

	std::cout << "Configuring dongle… ";
	std::cout.flush();
	device.set_configuration(1);
	device.control_no_data(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT, 0x01, 0x16, 0, 0);
	device.control_no_data(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT, 0x03, 0x00, 0, 0);
	device.control_no_data(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT, 0x05, 0x1846, 0, 0);
	{
		static const uint64_t MAC = UINT64_C(0x20cb13bd834ab817);
		device.control_out(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT, 0x07, 0, 0, &MAC, sizeof(MAC), 0);
	}
	device.set_configuration(2);
	device.claim_interface(0);
	std::cout << "OK\n";

	uint8_t message_id = 0;

	std::cout << "Erasing flash… ";
	std::cout.flush();
	{
		const uint8_t PACKET[] = { static_cast<uint8_t>(robot), message_id, 0x04 };
		device.interrupt_out(2, PACKET, sizeof(PACKET), 0);
	}
	check_mdr(device, message_id);
	++message_id;
	{
		bool done = false;
		while (!done) {
			uint8_t message[64];
			std::size_t len = device.interrupt_in(2, message, sizeof(message), 0);
			if (len > 0) {
				if (message[0] == robot) {
					if (len == 2) {
						if (message[1] == 0x02) {
							done = true;
						}
					}
				}
			}
		}
	}
	std::cout << "OK\n";

	for (std::size_t i = 0; i < hex.data()[0].size(); i += 256) {
		std::cout << "\rProgramming page " << (i / 256 + 1) << '/' << ((hex.data()[0].size() + 255) / 256) << "… ";
		std::cout.flush();
		uint8_t page[256];
		std::memset(page, 0xFF, sizeof(page));
		std::memcpy(page, &hex.data()[0][i], std::min<std::size_t>(hex.data()[0].size() - i, 256U));
		for (std::size_t j = 0; j < 256; j += 60) {
			std::size_t block_size = 60;
			if (j + block_size > 256) {
				block_size = 256 - j;
			}
			uint8_t block[4 + block_size];
			block[0] = static_cast<uint8_t>(robot);
			block[1] = message_id;
			block[2] = 0x05;
			block[3] = static_cast<uint8_t>(j);
			std::memcpy(block + 4, page + j, block_size);
			device.interrupt_out(2, block, sizeof(block), 0);

			check_mdr(device, message_id);
			++message_id;
		}
		{
			const uint8_t message[] = { static_cast<uint8_t>(robot), message_id, 0x06, static_cast<uint8_t>(i >> 8), static_cast<uint8_t>(i >> 16) };
			device.interrupt_out(2, message, sizeof(message), 0);

			check_mdr(device, message_id);
			++message_id;
		}
	}
	std::cout << "OK\n";

	for (std::size_t i = 0; i < hex.data()[0].size(); i += 256) {
		std::cout << "\rChecking page " << (i / 256 + 1) << '/' << ((hex.data()[0].size() + 255) / 256) << "… ";
		std::cout.flush();
		uint8_t page[256];
		std::memset(page, 0xFF, sizeof(page));
		std::memcpy(page, &hex.data()[0][i], std::min<std::size_t>(hex.data()[0].size() - i, 256U));
		uint32_t sum_computed = 0;
		for (std::size_t j = 0; j < 256; ++j) {
			sum_computed += page[j];
		}
		{
			const uint8_t message[] = { static_cast<uint8_t>(robot), message_id, 0x07, static_cast<uint8_t>(i), static_cast<uint8_t>(i >> 8), static_cast<uint8_t>(i >> 16), 0, 1, 0 };
			device.interrupt_out(2, message, sizeof(message), 0);
		}
		check_mdr(device, message_id);
		++message_id;
		uint32_t sum_received = 0;
		{
			bool done = false;
			while (!done) {
				uint8_t message[64];
				std::size_t len = device.interrupt_in(2, message, sizeof(message), 0);
				if (len > 0) {
					if (message[0] == robot) {
						if (len == 6) {
							if (message[1] == 0x03) {
								sum_received = message[2] | (message[3] << 8) | (message[4] << 16) | (message[5] << 24);
								done = true;
							}
						}
					}
				}
			}
		}
		if (sum_received != sum_computed) {
			device.release_interface(0);
			device.set_configuration(1);
			throw std::runtime_error(Glib::locale_from_utf8(Glib::ustring::compose(u8"Check failed on page %1: expected %2 but got %3", i / 256, sum_computed, sum_received)).c_str());
		}
	}
	std::cout << "OK\n";
	device.release_interface(0);
	device.set_configuration(1);
}

