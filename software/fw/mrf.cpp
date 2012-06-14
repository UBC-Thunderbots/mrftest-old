#include "fw/mrf.h"
#include "fw/ihex.h"
#include "util/libusb.h"
#include <iostream>

void Firmware::mrf_upload(const IntelHex &hex, unsigned int robot) {
	std::cout << "Opening dongle… ";
	std::cout.flush();
	USB::Context context;
	USB::DeviceHandle device(context, 0xC057, 0x2579);
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

	std::cout << "Erasing flash… ";
	std::cout.flush();
	{
		const uint8_t PACKET[] = { static_cast<uint8_t>(robot), 0x00, 0x04 };
		device.interrupt_out(2, PACKET, sizeof(PACKET), 0);
	}
	std::cout << "Submitted\n";
	{
		bool done = false;
		while (!done) {
			uint8_t mdr[2];
			std::size_t len = device.interrupt_in(1, mdr, sizeof(mdr), 0);
			std::cout << "len=" << len << '\n';
			if (len == sizeof(mdr)) {
				std::cout << "mdr[0]=" << (mdr[0] + 0U) << '\n';
				if (mdr[0] == 0x00) {
					std::cout << "mdr[1]=" << (mdr[1] + 0U) << '\n';
					switch (mdr[1]) {
						case 0x00: done = true; break;
						case 0x01: std::cout << "Robot not associated!\n"; device.set_configuration(1); return;
						case 0x02: std::cout << "Not acknowledged!\n"; device.set_configuration(1); return;
						case 0x03: std::cout << "Clear channel error!\n"; device.set_configuration(1); return;
						default: std::cout << "Unknown message delivery status!\n"; device.set_configuration(1); return;
					}
				}
			} else if (len != 0) {
				std::cout << "Bad USB transfer size!\n";
				device.set_configuration(1);
				return;
			}
		}
	}
	std::cout << "MDR OK\n";
	{
		bool done = false;
		while (!done) {
			uint8_t message[64];
			std::size_t len = device.interrupt_in(2, message, sizeof(message), 0);
			std::cout << "len=" << len << '\n';
			if (len > 0) {
				std::cout << "message[0]=" << (message[0] + 0U) << '\n';
				if (message[0] == robot) {
					if (len == 2) {
						std::cout << "message[1]=" << (message[1] + 0U) << '\n';
						if (message[1] == 0x02) {
							done = true;
						}
					}
				}
			}
		}
	}
	std::cout << "OK\n";
}

