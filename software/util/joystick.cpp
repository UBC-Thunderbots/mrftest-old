#include "util/joystick.h"
#include "util/algorithm.h"
#include "util/exception.h"
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <climits>
#include <cstdint>
#include <fcntl.h>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <utility>
#include <glibmm/convert.h>
#include <glibmm/fileutils.h>
#include <glibmm/main.h>
#include <glibmm/stringutils.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace {
	class NotJoystickError final : std::runtime_error {
		public:
			explicit NotJoystickError() : std::runtime_error("Event node is not a joystick") {
			}
	};

	constexpr std::size_t BITS_PER_UINT = CHAR_BIT * sizeof(unsigned int);

	constexpr std::size_t bits_to_uints(std::size_t bits) {
		return (bits + BITS_PER_UINT - 1U) / BITS_PER_UINT;
	}

	bool array_test_bit(const unsigned int *bit_array, std::size_t bit) {
		return !!(bit_array[bit / BITS_PER_UINT] & (1U << (bit % BITS_PER_UINT)));
	}

	FileDescriptor open_joystick(unsigned int index) {
		// Open the device.
		std::string path = "/dev/input/event";
		path.append(std::to_string(index));
		FileDescriptor fd = FileDescriptor::create_open(path.c_str(), O_RDONLY | O_NONBLOCK, 0);

		// Verify that it supports the expected version of the event API.
		int version;
		if (ioctl(fd.fd(), EVIOCGVERSION, &version) < 0) {
			throw SystemError("ioctl(EVIOCGVERSION)", errno);
		}
		if (version != EV_VERSION) {
			throw std::runtime_error("Event API version incorrect");
		}

		// Verify that it has both absolute axes and keys.
		unsigned int bits[bits_to_uints(EV_CNT)];
		if (ioctl(fd.fd(), EVIOCGBIT(0, sizeof(bits)), bits) < 0) {
			throw SystemError("ioctl(EVIOCGBIT(0))", errno);
		}
		if (!array_test_bit(bits, EV_ABS) || !array_test_bit(bits, EV_KEY)) {
			throw NotJoystickError();
		}

		// Verify that it is either a joystick or a gamepad.
		unsigned int keys[bits_to_uints(KEY_CNT)];
		if (ioctl(fd.fd(), EVIOCGBIT(EV_KEY, sizeof(keys)), keys) < 0) {
			throw SystemError("ioctl(EVIOCGBIT(EV_KEY))", errno);
		}
		if (!array_test_bit(keys, BTN_JOYSTICK) && !array_test_bit(keys, BTN_GAMEPAD)) {
			throw NotJoystickError();
		}

		return fd;
	}

	const Glib::ustring get_text(const FileDescriptor &fd, std::function<unsigned long(std::size_t)> iocidfn) {
		std::vector<char> buffer(8U);
		int ret;
		do {
			std::size_t new_size = buffer.size() * 2U;
			buffer.clear();
			buffer.resize(new_size);
			ret = ioctl(fd.fd(), iocidfn(buffer.size()), &buffer[0U]);
			if (ret < 0) {
				throw SystemError("ioctl(EVIOCGNAME)", errno);
			}
		} while (static_cast<std::size_t>(ret) == buffer.size());
		buffer.resize(static_cast<std::size_t>(ret - 1));
		return Glib::locale_to_utf8(std::string(buffer.begin(), buffer.end()));
	}

	void get_id(const FileDescriptor &fd, uint16_t &bus_type, uint16_t &vendor_id, uint16_t &product_id, uint16_t &version) {
		input_id id;
		if (ioctl(fd.fd(), EVIOCGID, &id) < 0) {
			throw SystemError("ioctl(EVIOCGID)", errno);
		}
		bus_type = id.bustype;
		vendor_id = id.vendor;
		product_id = id.product;
		version = id.version;
	}

	void make_map(std::unordered_map<unsigned int, unsigned int> &m, const FileDescriptor &fd, unsigned long ioc, const char *iocname, unsigned int bit_count) {
		unsigned int bits[bits_to_uints(bit_count)];
		if (ioctl(fd.fd(), ioc, bits) < 0) {
			throw SystemError(iocname, errno);
		}
		unsigned int next_index = 0U;
		for (unsigned int i = 0U; i != bit_count; ++i) {
			if (array_test_bit(bits, i)) {
				m[i] = next_index++;
			}
		}
	}
}

std::vector<std::unique_ptr<Joystick>> &Joystick::instances() {
	static std::vector<std::unique_ptr<Joystick>> sticks;
	static bool initialized = false;
	if (!initialized) {
		sticks.clear();
		Glib::Dir dir("/dev/input");
		std::vector<unsigned int> node_indices;
		for (auto node : dir) {
			if (Glib::str_has_prefix(node, "event")) {
				try {
					std::size_t end;
					unsigned long index = std::stoul(node.substr(5U), &end, 10);
					if (end == node.size() - 5U && index <= std::numeric_limits<unsigned int>::max()) {
						node_indices.push_back(static_cast<unsigned int>(index));
					} else {
						std::wcout << Glib::ustring::compose(u8"Joystick: device node \"/dev/input/%1\" does not match pattern \"event#\"\n", node);
					}
				} catch (const std::invalid_argument &exp) {
					std::wcout << Glib::ustring::compose(u8"Joystick: device node \"/dev/input/%1\" does not match pattern \"event#\"\n", node);
				} catch (const std::out_of_range &exp) {
					std::wcout << Glib::ustring::compose(u8"Joystick: device node \"/dev/input/%1\" does not match pattern \"event#\"\n", node);
				}
			}
		}
		std::sort(node_indices.begin(), node_indices.end());
		for (unsigned int index : node_indices) {
			try {
				std::unique_ptr<Joystick> ptr(new Joystick(index));
				sticks.push_back(std::move(ptr));
			} catch (const SystemError &exp) {
				if (exp.error_code != EACCES) {
					std::wcout << Glib::ustring::compose(u8"Cannot open \"/dev/input/event%1\": %2\n", index, exp.what());
				}
				// Swallow EACCES silently as it will happen for keyboards/mice/etc.
			} catch (const NotJoystickError &exp) {
				// Swallow silently as it will happen for lots of devices, if we have permissions on them.
			}
		}
		initialized = true;
	}
	return sticks;
}

Joystick::Joystick(unsigned int index) : fd(open_joystick(index)), flushing_dropped(false) {
	// Grab device identification.
	identifier_.name = get_text(fd, [](std::size_t len) { return EVIOCGNAME(len); });
	physical_location_ = get_text(fd, [](std::size_t len) { return EVIOCGPHYS(len); });
	get_id(fd, identifier_.bus_type, identifier_.vendor_id, identifier_.product_id, identifier_.version);

	// Build maps.
	make_map(axes_map, fd, EVIOCGBIT(EV_ABS, static_cast<int>(bits_to_uints(ABS_CNT) * sizeof(unsigned int))), "EVIOCGBIT(EV_ABS)", ABS_CNT);
	make_map(buttons_map, fd, EVIOCGBIT(EV_KEY, static_cast<int>(bits_to_uints(KEY_CNT) * sizeof(unsigned int))), "EVIOCGBIT(EV_KEY)", KEY_CNT);

	// Build data stores.
	// This slightly odd construction is due to the fact that Property objects are movable but not copyable.
	// Thus, one cannot just use resize(N, X) because vector can't make N copies of X.
	axes_metadata.resize(axes_map.size());
	for (std::size_t i = axes_map.size(); i--;) {
		axes_.push_back(Property<double>(0));
	}
	for (std::size_t i = buttons_map.size(); i--;) {
		buttons_.push_back(Property<bool>(false));
	}

	// Retrieve initial values.
	read_all_axes();
	read_all_buttons();

	// Connect I/O signal.
	Glib::signal_io().connect(sigc::bind_return(sigc::hide(sigc::mem_fun(this, &Joystick::on_readable)), true), fd.fd(), Glib::IO_IN);
}

void Joystick::on_readable() {
	bool changed = false;
	for (;;) {
		input_event events[64U];
		ssize_t rc = read(fd.fd(), events, sizeof(events));
		if (rc < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				break;
			} else {
				throw SystemError("read(joystick)", errno);
			}
		} else if (!rc) {
			throw std::runtime_error("Unexpected EOF reading joystick");
		}
		for (std::size_t i = 0U; i * sizeof(input_event) < static_cast<std::size_t>(rc); ++i) {
			const input_event &event = events[i];
			if (flushing_dropped) {
				if (event.type == EV_SYN && event.code == SYN_REPORT) {
					flushing_dropped = false;
					read_all_axes();
					read_all_buttons();
					changed = true;
				}
			} else {
				switch (event.type) {
					case EV_SYN:
						if (event.code == SYN_DROPPED) {
							flushing_dropped = true;
							read_all_axes();
							read_all_buttons();
							changed = true;
						}
						break;

					case EV_ABS:
						{
							unsigned int index = axes_map[event.code];
							double new_value = convert_axis(event.value, axes_metadata[index]);
							if (new_value != axes_[index]) {
								axes_[index] = new_value;
								changed = true;
							}
						}
						break;

					case EV_KEY:
						{
							unsigned int index = buttons_map[event.code];
							bool new_value = event.value != 0;
							if (new_value != buttons_[index]) {
								buttons_[index] = new_value;
								changed = true;
							}
						}
						break;
				}
			}
		}
	}

	if (changed) {
		signal_changed_.emit();
	}
}

void Joystick::read_all_axes() {
	for (auto mapping : axes_map) {
		input_absinfo data;
		if (ioctl(fd.fd(), EVIOCGABS(mapping.first), &data) < 0) {
			throw SystemError("EVIOCGABS", errno);
		}

		AxisMetadata &md = axes_metadata[mapping.second];
		md.minimum = data.minimum;
		md.maximum = data.maximum;
		md.flat = data.flat;
		axes_[mapping.second] = convert_axis(data.value, md);
	}
}

void Joystick::read_all_buttons() {
	unsigned int keys[bits_to_uints(KEY_CNT)];
	if (ioctl(fd.fd(), EVIOCGKEY(sizeof(keys)), keys) < 0) {
		throw SystemError("EVIOCGKEY", errno);
	}

	for (auto mapping : buttons_map) {
		buttons_[mapping.second] = array_test_bit(keys, mapping.first);
	}
}

double Joystick::convert_axis(int32_t value, const AxisMetadata &md) {
	int32_t centre = (md.minimum + md.maximum) / 2;
	value -= centre;
	int32_t range = md.maximum - centre;
	value = clamp(value, -range, range);
	if (-md.flat <= value && value <= md.flat) {
		return 0.0;
	} else {
		if (value < 0) {
			value += md.flat;
		} else {
			value -= md.flat;
		}
		range -= md.flat;
		return static_cast<double>(value) / range;
	}
}

