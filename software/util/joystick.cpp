#include "util/joystick.h"
#include "util/exception.h"
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <fcntl.h>
#include <stdint.h>
#include <string>
#include <unistd.h>
#include <utility>
#include <glibmm/convert.h>
#include <glibmm/fileutils.h>
#include <glibmm/main.h>
#include <linux/joystick.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

namespace {
	FileDescriptor open_joystick(const std::string &node) {
		const std::string &path = "/dev/input/" + node;
		FileDescriptor fd = FileDescriptor::create_open(path.c_str(), O_RDONLY | O_NONBLOCK, 0);
		uint32_t version;
		if (ioctl(fd.fd(), JSIOCGVERSION, &version) < 0) {
			throw SystemError("ioctl(JSIOCGVERSION)", errno);
		}
		if (version < 0x020100) {
			throw std::runtime_error("Joystick API version too low");
		}
		return fd;
	}

	Glib::ustring get_name(const FileDescriptor &fd) {
		std::vector<char> buffer;
		int len;
		do {
			buffer.resize(std::max<std::size_t>(128, buffer.size() * 2));
			if ((len = ioctl(fd.fd(), JSIOCGNAME(buffer.size()), &buffer[0])) < 0) {
				throw SystemError("ioctl(JSIOCGNAME)", errno);
			}
			if (!len) {
				return "";
			}
		} while (len == static_cast<int>(buffer.size()));
		return Glib::locale_to_utf8(std::string(&buffer[0], static_cast<std::size_t>(len) - 1));
	}

	unsigned int get_num_objects(const FileDescriptor &fd, unsigned long req, const char *call) {
		uint8_t u8;
		if (ioctl(fd.fd(), req, &u8) < 0) {
			throw SystemError(call, errno);
		}
		return u8;
	}

	unsigned int get_num_axes(const FileDescriptor &fd) {
		return get_num_objects(fd, JSIOCGAXES, "ioctl(JSIOCGAXES)");
	}

	unsigned int get_num_buttons(const FileDescriptor &fd) {
		return get_num_objects(fd, JSIOCGBUTTONS, "ioctl(JSIOCGBUTTONS)");
	}
}

std::vector<std::unique_ptr<Joystick>> &Joystick::instances() {
	static std::vector<std::unique_ptr<Joystick>> sticks;
	static bool initialized = false;
	if (!initialized) {
		sticks.clear();
		Glib::Dir dir("/dev/input");
		std::vector<std::string> names(dir.begin(), dir.end());
		std::sort(names.begin(), names.end());
		for (auto i = names.begin(), iend = names.end(); i != iend; ++i) {
			const std::string &node = *i;
			if (node.size() > 2 && node[0] == 'j' && node[1] == 's') {
				std::unique_ptr<Joystick> ptr(new Joystick(node));
				sticks.push_back(std::move(ptr));
			}
		}
		initialized = true;
	}
	return sticks;
}

Joystick::Joystick(const std::string &node) : fd(open_joystick(node)), node_(node), name_(get_name(fd)) {
	// This slightly odd construction is due to the fact that Property objects are movable but not copyable.
	// Thus, one cannot just use resize(N, X) because vector can't make N copies of X.
	for (unsigned int i = get_num_axes(fd); i--;) {
		axes_.push_back(Property<double>(0));
	}
	for (unsigned int i = get_num_buttons(fd); i--;) {
		buttons_.push_back(Property<bool>(false));
	}
	Glib::signal_io().connect(sigc::bind_return(sigc::hide(sigc::mem_fun(this, &Joystick::on_readable)), true), fd.fd(), Glib::IO_IN);
}

void Joystick::on_readable() {
	for (;;) {
		js_event buffer[8];
		ssize_t rc = read(fd.fd(), buffer, sizeof(buffer));
		if (rc < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				signal_changed().emit();
				return;
			} else {
				throw SystemError("read(joystick)", errno);
			}
		} else if (!rc) {
			throw std::runtime_error("Unexpected EOF reading joystick");
		}
		for (std::size_t i = 0; i * sizeof(js_event) < static_cast<std::size_t>(rc); ++i) {
			if ((buffer[i].type & ~JS_EVENT_INIT) == JS_EVENT_AXIS) {
				assert(buffer[i].number < axes_.size());
				axes_[buffer[i].number] = buffer[i].value / 32767.0;
			} else if ((buffer[i].type & ~JS_EVENT_INIT) == JS_EVENT_BUTTON) {
				assert(buffer[i].number < buttons_.size());
				buttons_[buffer[i].number] = !!buffer[i].value;
			}
		}
	}
}

