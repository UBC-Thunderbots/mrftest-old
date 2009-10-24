#include "robot_controller/joystick/joystick.h"
#include <stdexcept>
#include <string>
#include <cassert>
#include <cerrno>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/joystick.h>
#include <unistd.h>

namespace {
	int open_device(const Glib::ustring &filename) {
		const std::string &fn = Glib::filename_from_utf8(filename);
		int desc = open(fn.c_str(), O_RDONLY | O_NONBLOCK);
		if (desc < 0)
			throw std::runtime_error("Error opening joystick!");
		return desc;
	}

	bool list_inited = false;
	std::vector<std::pair<Glib::ustring, Glib::ustring> > list_data;
}

joystick::joystick(const Glib::ustring &filename) : fd(open_device(filename)), stick_filename(filename) {
	char buffer[128];
	if (ioctl(fd, JSIOCGNAME(sizeof(buffer)), buffer) < 0)
		throw std::runtime_error("JSIOCGNAME failed!");
	stick_name = Glib::ustring(buffer);

	char ch;
	if (ioctl(fd, JSIOCGAXES, &ch) < 0)
		throw std::runtime_error("JSIOCGAXES failed!");
	axes_data.resize(ch, 0);

	if (ioctl(fd, JSIOCGBUTTONS, &ch) < 0)
		throw std::runtime_error("JSIOCGBUTTONS failed!");
	buttons_data.resize(ch, false);

	Glib::signal_io().connect(sigc::mem_fun(*this, &joystick::on_readable), fd, Glib::IO_IN);
}

bool joystick::on_readable(Glib::IOCondition) {
	js_event events[32];
	ssize_t len = read(fd, &events, sizeof(events));
	if (len < 0 && errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)
		throw std::runtime_error("Error reading from joystick!");

	for (unsigned int i = 0; i < len / sizeof(js_event); i++) {
		switch (events[i].type & ~JS_EVENT_INIT) {
			case JS_EVENT_AXIS:
				assert(events[i].number < axes_data.size());
				axes_data[events[i].number] = events[i].value;
				break;

			case JS_EVENT_BUTTON:
				assert(events[i].number < buttons_data.size());
				buttons_data[events[i].number] = !!events[i].value;
				break;
		}
	}

	return true;
}

const std::vector<std::pair<Glib::ustring, Glib::ustring> > &list() {
	if (!list_inited) {
		Glib::Dir dir("/dev/input");
		for (Glib::Dir::const_iterator i = dir.begin(), iend = dir.end(); i != iend; ++i) {
			const Glib::ustring &filename = Glib::filename_to_utf8(*i);
			if (filename[0] == 'j' && filename[1] == 's') {
				joystick js(filename);
				list_data.push_back(std::make_pair(filename, js.name()));
			}
		}
		list_inited = true;
	}

	return list_data;
}

