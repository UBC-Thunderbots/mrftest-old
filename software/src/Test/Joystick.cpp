#include "Log/Log.h"
#include "Test/Joystick.h"

#include <algorithm>
#include <string>
#include <vector>
#include <cerrno>
#include <cstring>

#include <sigc++/sigc++.h>
#include <glibmm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/joystick.h>

namespace {
	bool initialized = false;
	std::vector<Glib::RefPtr<Joystick> > sticks;

	bool joystickComparator(Glib::RefPtr<Joystick> x, Glib::RefPtr<Joystick> y) {
		return x->name() < y->name();
	}
}

sigc::signal<void> &Joystick::signal_changed() {
	return sig_changed;
}

const std::string &Joystick::name() const {
	return filename;
}

const std::vector<Glib::RefPtr<Joystick> > Joystick::all() {
	if (!initialized) {
		Glib::Dir dir("/dev/input");
		for (Glib::Dir::const_iterator i = dir.begin(), iend = dir.end(); i != iend; ++i) {
			std::string file = *i;
			if (file[0] == 'j' && file[1] == 's') {
				Glib::RefPtr<Joystick> js = Joystick::create("/dev/input/" + file);
				if (js)
					sticks.push_back(js);
			}
		}
		sort(sticks.begin(), sticks.end(), &joystickComparator);
		initialized = true;
	}
	return sticks;
}

Glib::RefPtr<Joystick> Joystick::create(const std::string &filename) {
	int fd = open(filename.c_str(), O_RDONLY | O_NONBLOCK);
	if (fd < 0) {
		int err = errno;
		Log::log(Log::LEVEL_ERROR, "Joystick") << "Cannot open joystick device " << filename << ": " << std::strerror(err) << '\n';
		throw;
	}

	unsigned char cnt;
	if (ioctl(fd, JSIOCGBUTTONS, &cnt) < 0) {
		int err = errno;
		Log::log(Log::LEVEL_ERROR, "Joystick") << "Cannot get number of buttons for " << filename << ": " << std::strerror(err) << '\n';
		close(fd);
		throw;
	}

	if (cnt != NUM_BTNS) {
		Log::log(Log::LEVEL_ERROR, "Joystick") << "Wrong number of buttons for " << filename << '\n';
		close(fd);
		throw;
	}

	if (ioctl(fd, JSIOCGAXES, &cnt) < 0) {
		int err = errno;
		Log::log(Log::LEVEL_ERROR, "Joystick") << "Cannot get number of axes for " << filename << ": " << std::strerror(err) << '\n';
		close(fd);
		throw;
	}

	if (cnt != NUM_AXES) {
		Log::log(Log::LEVEL_ERROR, "Joystick") << "Wrong number of axes for " << filename << '\n';
		close(fd);
		throw;
	}

	Glib::RefPtr<Joystick> js(new Joystick(fd, filename));
	return js;
}

void Joystick::reference() {
	refs++;
}

void Joystick::unreference() {
	if (!--refs)
		delete this;
}

Joystick::Joystick(int fd, const std::string &device) : fd(fd), filename(device), refs(1) {
	Glib::signal_io().connect(sigc::mem_fun(*this, &Joystick::onIO), fd, Glib::IO_IN);
}

Joystick::~Joystick() {
	close(fd);
}

bool Joystick::onIO(Glib::IOCondition cond) {
	if (cond & (Glib::IO_ERR | Glib::IO_HUP | Glib::IO_NVAL)) {
		Log::log(Log::LEVEL_ERROR, "Joystick") << "Device " << filename << " is broken.\n";
		return false;
	}

	struct js_event events[32];
	for (;;) {
		ssize_t ret = read(fd, &events, sizeof(events));
		if (ret > 0) {
			for (unsigned int i = 0; i < ret / sizeof(*events); i++) {
				events[i].type &= ~JS_EVENT_INIT;
				if (events[i].type == JS_EVENT_BUTTON) {
					buttons[events[i].number] = !!events[i].value;
				} else if (events[i].type == JS_EVENT_AXIS) {
					axes[events[i].number] = events[i].value;
				}
			}
			sig_changed();
		} else if (errno == EAGAIN || errno == EWOULDBLOCK) {
			break;
		} else {
			int err = errno;
			Log::log(Log::LEVEL_ERROR, "Joystick") << "Error reading from joystick device " << filename << ": " << std::strerror(err) << '\n';
			break;
		}
	}

	return true;
}

