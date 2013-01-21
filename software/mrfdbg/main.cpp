#include "util/libusb.h"
#include "util/string.h"
#include <iostream>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <typeinfo>
#include <vector>

namespace {
	unsigned long convert_number(const std::string &str) {
		std::size_t end_pos = 0;
		unsigned long ul = std::stoul(str, &end_pos, 0);
		if (end_pos != str.size()) {
			throw std::runtime_error("Whole string not consumed in number conversion");
		}
		return ul;
	}

	void split(const std::string &whole, std::vector<std::string> &parts) {
		if (whole.empty()) {
			return;
		}
		std::string::size_type idx = 0;
		while (idx != std::string::npos) {
			if (whole[idx] == ' ') {
				idx = whole.find_first_not_of(" ", idx);
			} else {
				std::string::size_type idx2 = whole.find_first_of(" ", idx);
				parts.emplace_back(whole, idx, idx2 == std::string::npos ? std::string::npos : idx2 - idx);
				idx = idx2;
			}
		}
	}

	int main_impl() {
		// Set the current locale from environment variables
		std::locale::global(std::locale(""));

		// Open the dongle
		std::cout << "Addressing dongle… ";
		std::cout.flush();
		USB::Context ctx;
		USB::DeviceHandle devh(ctx, 0xC057, 0x2579);
		std::cout << "OK\n";

		// Set configuration
		if (devh.get_configuration() == 6) {
			devh.set_configuration(1);
		}
		devh.set_configuration(6);
		devh.claim_interface(0);

		// Go into a loop
		for (;;) {
			std::cout << "> ";
			std::cout.flush();
			std::string line;
			if (!std::getline(std::cin, line)) {
				std::cout << "EOF\n";
				break;
			}
			std::vector<std::string> parts;
			split(line, parts);
			if (!parts.empty()) {
				if (parts[0] == "beep" && parts.size() == 2) {
					uint16_t millis = static_cast<uint16_t>(convert_number(parts[1]));
					devh.control_no_data(LIBUSB_REQUEST_TYPE_VENDOR, 0x12, millis, 0, 0);
				} else if (parts[0] == "gc" && parts.size() == 1) {
					// Get control lines
					uint8_t buffer;
					if (devh.control_in(LIBUSB_REQUEST_TYPE_VENDOR, 0x0C, 0x0000, 0x0000, &buffer, 1, 0) != 1) {
						throw std::runtime_error("Dongle returned short response");
					}
					std::cout << "¬RESET = " << !!(buffer & 0x01) << '\n';
					std::cout << "WAKE   = " << !!(buffer & 0x02) << '\n';
					std::cout << "INT    = " << !!(buffer & 0x04) << '\n';
					if (buffer & !0x07) {
						std::cout << "Extra bits: " << (buffer & ~0x07) << '\n';
					}
				} else if (parts[0] == "sc" && parts.size() == 3 && (parts[1] == "0" || parts[1] == "1") && (parts[2] == "0" || parts[2] == "1")) {
					// Set control lines
					uint8_t buffer = 0;
					if (parts[1] == "1") {
						buffer |= 0x01;
					}
					if (parts[2] == "1") {
						buffer |= 0x02;
					}
					devh.control_no_data(LIBUSB_REQUEST_TYPE_VENDOR, 0x0D, buffer, 0x0000, 0);
				} else if (parts[0] == "rs" && parts.size() == 2) {
					uint16_t addr = static_cast<uint16_t>(convert_number(parts[1]));
					uint8_t buffer;
					if (devh.control_in(LIBUSB_REQUEST_TYPE_VENDOR, 0x0E, 0x0000, addr, &buffer, 1, 0) != 1) {
						throw std::runtime_error("Dongle returned short response");
					}
					std::cout << "short[0x" << tohex(addr, 2) << "] = 0x" << tohex(buffer, 2) << '\n';
				} else if (parts[0] == "ws" && parts.size() == 3) {
					uint16_t addr = static_cast<uint16_t>(convert_number(parts[1]));
					uint16_t value = static_cast<uint16_t>(convert_number(parts[2]));
					devh.control_no_data(LIBUSB_REQUEST_TYPE_VENDOR, 0x0F, value, addr, 0);
				} else if (parts[0] == "rl" && parts.size() == 2) {
					uint16_t addr = static_cast<uint16_t>(convert_number(parts[1]));
					uint8_t buffer;
					if (devh.control_in(LIBUSB_REQUEST_TYPE_VENDOR, 0x10, 0x0000, addr, &buffer, 1, 0) != 1) {
						throw std::runtime_error("Dongle returned short response");
					}
					std::cout << "short[0x" << tohex(addr, 2) << "] = 0x" << tohex(buffer, 2) << '\n';
				} else if (parts[0] == "wl" && parts.size() == 3) {
					uint16_t addr = static_cast<uint16_t>(convert_number(parts[1]));
					uint16_t value = static_cast<uint16_t>(convert_number(parts[2]));
					devh.control_no_data(LIBUSB_REQUEST_TYPE_VENDOR, 0x11, value, addr, 0);
				} else if (parts[0] == "wi" && parts.size() == 2) {
					unsigned int seconds = static_cast<unsigned int>(convert_number(parts[1]));
					uint8_t buffer;
					try {
						std::size_t len = devh.interrupt_in(1, &buffer, 1, seconds * 1000);
						if (len == 1) {
							if (buffer == 0x00) {
								std::cout << "INT falling edge\n";
							} else if (buffer == 0x01) {
								std::cout << "INT rising edge\n";
							} else {
								std::cout << "Unknown value " << (buffer + 0U) << "\n";
							}
						}
					} catch (const USB::TransferTimeoutError &exp) {
						std::cout << "Timeout\n";
					}
				} else {
					std::cout << "Unrecognized command or invalid arguments; valid commands are:\n";
					std::cout << "beep millis: activate the buzzer for <millis> milliseconds\n";
					std::cout << "gc: get control lines\n";
					std::cout << "sc {0|1} {0|1}: set control lines, first ¬RESET then WAKE\n";
					std::cout << "rs addr: read short register\n";
					std::cout << "ws addr value: write short register\n";
					std::cout << "rl addr: read long register\n";
					std::cout << "wl addr value: write long register\n";
					std::cout << "wi seconds: wait for interrupt pin to change up to <seconds> seconds\n";
				}
			}
		}

		// Shut down the dongle
		devh.release_interface(0);
		devh.set_configuration(1);

		return 0;
	}
}

int main() {
	try {
		return main_impl();
	} catch (const std::exception &exp) {
		std::cerr << typeid(exp).name() << ": " << exp.what() << '\n';
	} catch (...) {
		std::cerr << "Unknown error!\n";
	}
	return 1;
}

