#include "datapool/Config.h"
#include "datapool/HWRunSwitch.h"
#include "Log/Log.h"

#include <vector>
#include <fstream>
#include <string>
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <cassert>

#include <glibmm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

// How long to wait before assuming the switch is broken if it sends no data.
#define RECEIVE_TIMEOUT 1000

namespace {
	//
	// All the data about a single run switch
	//
	class Device;
	typedef Glib::RefPtr<Device> PDevice;
	typedef std::vector<PDevice> VDevice;
	class Device : public Glib::Object {
	public:
		//
		// The states the switch can be in.
		//
		enum State {
			// The switch is in the KILL position.
			KILL,
			// The switch is in the RUN position.
			RUN,
			// Communication with the switch failed.
			BROKEN,
		};

		//
		// Creats a new device given the filename of its TTY node.
		//
		static PDevice create(const std::string &device);

		//
		// The property indicating the current state of the switch.
		//
		Glib::PropertyProxy<State> property_state();

	private:
		Glib::Property<State> prop_state;
		std::string device;
		int fd;
		unsigned int refs;
		sigc::connection ioConnection, timeoutConnection;

		Device(const std::string &device, int fd);
		Device(const Device &copyref); // Prohibit copying.
		~Device();
		bool ioReady(Glib::IOCondition cond);
		bool timeout();
		void reference();
		void unreference();

		friend class Glib::RefPtr<Device>;
	};

	//
	// The switches.
	//
	VDevice switches;
}

PDevice Device::create(const std::string &device) {
	// Open serial port.
	int serialPort = open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (serialPort < 0) {
		int err = errno;
		Log::log(Log::LEVEL_WARNING, "Run Switch") << "Cannot open " << device << ": " << std::strerror(err) << " (continuing anyway)\n";
		return PDevice();
	}

	// Initialize TTY settings.
	termios tios;
	if (tcgetattr(serialPort, &tios) < 0) {
		int err = errno;
		Log::log(Log::LEVEL_WARNING, "Run Switch") << "Cannot configure " << device << ": " << std::strerror(err) << " (continuing anyway)\n";
		close(serialPort);
		return PDevice();
	}
	tios.c_iflag = 0;
	tios.c_oflag = NL0 | CR0 | TAB0 | BS0 | VT0 | FF0;
	tios.c_cflag = CS8 | CSTOPB | CREAD | HUPCL | CRTSCTS;
	tios.c_lflag = 0;
	tios.c_cc[VMIN] = 1;
	tios.c_cc[VTIME] = 0;
	cfsetispeed(&tios, B115200);
	cfsetospeed(&tios, B115200);
	if (tcsetattr(serialPort, TCSAFLUSH, &tios) < 0) {
		int err = errno;
		Log::log(Log::LEVEL_WARNING, "Run Switch") << "Cannot configure " << device << ": " << std::strerror(err) << " (continuing anyway)\n";
		close(serialPort);
		return PDevice();
	}

	Log::log(Log::LEVEL_INFO, "Run Switch") << "Opened switch at " << device << '\n';

	PDevice dev(new Device(device, serialPort));
	return dev;
}

Glib::PropertyProxy<Device::State> Device::property_state() {
	return prop_state.get_proxy();
}

Device::Device(const std::string &device, int fd) : Glib::ObjectBase(typeid(Device)), prop_state(*this, "state", Device::KILL), device(device), fd(fd), refs(1) {
	ioConnection = Glib::signal_io().connect(sigc::mem_fun(this, &Device::ioReady), fd, Glib::IO_IN);
	timeoutConnection = Glib::signal_timeout().connect(sigc::mem_fun(this, &Device::timeout), RECEIVE_TIMEOUT);
}

Device::~Device() {
	ioConnection.disconnect();
	timeoutConnection.disconnect();
	close(fd);
}

bool Device::ioReady(Glib::IOCondition cond) {
	if (cond & Glib::IO_IN) {
		// There is data waiting. Read it from the device.
		char buf[32];
		ssize_t ret = read(fd, buf, sizeof(buf));
		if (ret > 0) {
			if (buf[ret - 1] == '0') {
				// Switch is in KILL state.
				prop_state = KILL;
			} else if (buf[ret - 1] == '1') {
				// Switch is in RUN state.
				prop_state = RUN;
			} else {
				// Switch is broken.
				prop_state = BROKEN;
				Log::log(Log::LEVEL_ERROR, "Run Switch") << device << " returned unknown switch state.\n";
			}
			timeoutConnection.disconnect();
			timeoutConnection = Glib::signal_timeout().connect(sigc::mem_fun(*this, &Device::timeout), RECEIVE_TIMEOUT);
		} else if (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
			// No data is available right now.
		} else {
			// An error occurred.
			int err = errno;
			Log::log(Log::LEVEL_ERROR, "Run Switch") << "Error reading from " << device << ": " << std::strerror(err) << '\n';
			prop_state = BROKEN;
		}
	} else if ((cond & Glib::IO_ERR) || (cond & Glib::IO_NVAL)) {
		// An error occurred.
		Log::log(Log::LEVEL_ERROR, "Run Switch") << "Error reading from " << device << ": bad file descriptor\n";
		prop_state = BROKEN;
		return false;
	} else if (cond & Glib::IO_HUP) {
		// The file was closed.
		Log::log(Log::LEVEL_ERROR, "Run Switch") << device << " was disconnected\n";
		prop_state = BROKEN;
		return false;
	}

	return true;
}

bool Device::timeout() {
	// Data was NOT received recently. The switch is broken.
	prop_state = BROKEN;
	return false;
}

void Device::reference() {
	refs++;
}

void Device::unreference() {
	assert(refs > 0);
	if (!--refs)
		delete this;
}

HWRunSwitch::HWRunSwitch() : Glib::ObjectBase(typeid(HWRunSwitch)), prop_state(*this, "state", false) {
	// Only one instance should exist at a time.
	assert(!inst);

	// Use device names from config file..
	for (unsigned int i = 0; ; i++) {
		std::ostringstream oss;
		oss << "Device" << i;
		std::string key = oss.str();
		if (!Config::instance().hasKey("HWRunSwitch", key))
			break;
		PDevice dev = Device::create(Config::instance().getString("HWRunSwitch", key));
		if (dev) {
			dev->property_state().signal_changed().connect(sigc::mem_fun(*this, &HWRunSwitch::onChange));
			switches.push_back(dev);
		}
	}

	// If there are no switches, allow the bots to run.
	if (switches.empty())
		prop_state = true;

	// Record instance.
	inst = this;
}

HWRunSwitch::~HWRunSwitch() {
	// Record destruction of global instance.
	inst = 0;
}

HWRunSwitch *HWRunSwitch::inst;

HWRunSwitch &HWRunSwitch::instance() {
	assert(inst);
	return *inst;
}

Glib::PropertyProxy<bool> HWRunSwitch::property_state() {
	return prop_state.get_proxy();
}

void HWRunSwitch::onChange() {
	// If any button is in KILL state, do not allow the robots to run.
	for (VDevice::const_iterator i = switches.begin(), iend = switches.end(); i != iend; ++i) {
		if ((*i)->property_state() == Device::KILL) {
			prop_state = false;
			return;
		}
	}

	// If any button is in RUN state, allow the robots to run.
	for (VDevice::const_iterator i = switches.begin(), iend = switches.end(); i != iend; ++i) {
		if ((*i)->property_state() == Device::RUN) {
			prop_state = true;
			return;
		}
	}

	// All the buttons must be broken. Do not allow the robots to run.
	prop_state = false;
}

