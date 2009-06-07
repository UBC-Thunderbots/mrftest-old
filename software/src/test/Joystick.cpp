#include "Log/Log.h"
#include "test/Joystick.h"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <linux/joystick.h>

std::vector<std::string> Joystick::list() {
	std::vector<std::string> lst;

	DIR *dir = opendir("/dev/input");
	if (!dir)
		return lst;

	struct dirent *de;
	while (de = readdir(dir)) {
		if (de->d_name[0] == 'j' && de->d_name[1] == 's') {
			lst.push_back(std::string("/dev/input/") + de->d_name);
		}
	}

	closedir(dir);

	sort(lst.begin(), lst.end());

	return lst;
}

Joystick::Joystick(const std::string &filename) : filename(filename) {
	fd = open(filename.c_str(), O_RDONLY | O_NONBLOCK);
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

	std::fill(buttons, buttons + NUM_BTNS, false);
	std::fill(axes, axes + NUM_AXES, 0);

	Log::log(Log::LEVEL_INFO, "Joystick") << "Opened device " << filename << '\n';
}

Joystick::~Joystick() {
	close(fd);
}

bool Joystick::update() {
	struct js_event events[32];
	bool retval = false;
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
			retval |= true;
		} else if (errno == EAGAIN || errno == EWOULDBLOCK) {
			break;
		} else {
			int err = errno;
			Log::log(Log::LEVEL_ERROR, "Joystick") << "Error reading from joystick device " << filename << ": " << std::strerror(err) << '\n';
			break;
		}
	}
	return retval;
}

