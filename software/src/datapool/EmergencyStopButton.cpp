#include "datapool/EmergencyStopButton.h"
#include "Log/Log.h"
#include <string>
#include <fstream>
#include <vector>
#include <tr1/memory>
#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <poll.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <time.h>

#define TIMEOUT 500

namespace {
	/*
	 * Gets the current timestamp, in milliseconds.
	 */
	unsigned long millis() {
		timespec ts;
		clock_gettime(CLOCK_MONOTONIC, &ts);
		return ts.tv_sec * 1000UL + ts.tv_nsec / 1000000UL;
	}

	/*
	 * All the data about a single stop button.
	 */
	class Button {
	public:
		enum State {
			KILL,
			RUN,
			BROKEN,
		};

		static std::tr1::shared_ptr<Button> create(const std::string &device);
		~Button();
		void update();
		State state() const { return st; }

	private:
		std::string device;
		int fd;
		unsigned long lastTime;
		State st;

		Button(const std::string &device, int fd);
		Button(const Button &copyref);
	};

	/*
	 * The buttons.
	 */
	std::vector<std::tr1::shared_ptr<Button> > buttons;
}

std::tr1::shared_ptr<Button> Button::create(const std::string &device) {
	// Open serial port.
	int serialPort = open(device.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (serialPort < 0) {
		int err = errno;
		Log::log(Log::LEVEL_ERROR, "EStop") << "open(" << device << "): " << std::strerror(err) << '\n';
		return std::tr1::shared_ptr<Button>();
	}

	// Initialize TTY settings.
	termios tios;
	if (tcgetattr(serialPort, &tios) < 0) {
		int err = errno;
		Log::log(Log::LEVEL_ERROR, "EStop") << "tcgetattr(" << device << "): " << std::strerror(err) << '\n';
		close(serialPort);
		serialPort = -1;
		return std::tr1::shared_ptr<Button>();
	}

	tios.c_iflag = 0;
	tios.c_oflag = NL0 | CR0 | TAB0 | BS0 | VT0 | FF0;
	tios.c_cflag = CS8 | CSTOPB | CREAD | HUPCL | CRTSCTS;
	tios.c_lflag = 0;
	tios.c_cc[VMIN] = 1;
	tios.c_cc[VTIME] = 0;
	cfsetispeed(&tios, B9600);
	cfsetospeed(&tios, B9600);
	if (tcsetattr(serialPort, TCSAFLUSH, &tios) < 0) {
		int err = errno;
		Log::log(Log::LEVEL_ERROR, "EStop") << "tcsetattr(" << device << "): " << std::strerror(err) << '\n';
		close(serialPort);
		serialPort = -1;
		return std::tr1::shared_ptr<Button>();
	}

	Log::log(Log::LEVEL_INFO, "EStop") << "Opened EStop at " << device << '\n';

	std::tr1::shared_ptr<Button> btn(new Button(device, serialPort));
	return btn;
}

Button::~Button() {
	close(fd);
}

void Button::update() {
	char tmp[32];
	ssize_t len;
	do {
		len = ::read(fd, tmp, sizeof(tmp));
	} while (len < 0 && errno == EINTR);

	if (len > 0) {
		// Handle it!
		st = tmp[len - 1] == '1' ? RUN : KILL;
		lastTime = millis();
	} else if (len == 0) {
		// Do nothing.
	} else if (errno == EAGAIN) {
		// Do nothing.
	} else {
		int err = errno;
		Log::log(Log::LEVEL_WARNING, "EStop") << "read(" << device << "): " << std::strerror(err) << '\n';
		st = BROKEN;
		return;
	}

	if (millis() - lastTime > TIMEOUT) {
		Log::log(Log::LEVEL_WARNING, "EStop") << "Timeout waiting for sample from " << device << '\n';
		st = BROKEN;
	}
}

Button::Button(const std::string &device, int fd) : device(device), fd(fd) {
}

bool EmergencyStopButton::state = true;

void EmergencyStopButton::init() {
	// Close already-open ports.
	buttons.clear();

	// Load configuration file.
	std::string device;
	{
		const char *homedir = std::getenv("HOME");
		if (!homedir) {
			Log::log(Log::LEVEL_ERROR, "EStop") << "Environment variable $HOME is not set!\n";
			std::exit(1);
		}
		std::string configfile(homedir);
		configfile += "/.thunderbots/estopbtn.conf";
		std::ifstream ifs(configfile.c_str());

		while (std::getline(ifs, device)) {
			std::tr1::shared_ptr<Button> btn = Button::create(device);
			if (btn)
				buttons.push_back(btn);
		}

		if (!ifs.good() && !ifs.eof()) {
			std::ostream &os = Log::log(Log::LEVEL_ERROR, "EStop");
			os << "Error reading file " << configfile << "; does it exist and is it properly formatted?\n";
			os << "It should contain the pathname to the TTY device\n";
			state = false;
			return;
		}
	}
}

void EmergencyStopButton::update() {
	// If there are no buttons attached, just set it in run state.
	if (buttons.empty()) {
		EmergencyStopButton::state = false;
		return;
	}

	// Update all buttons and accumulate state.
	Button::State state = Button::BROKEN;
	for (unsigned int i = 0; i < buttons.size(); i++) {
		buttons[i]->update();
		if (buttons[i]->state() < state)
			state = buttons[i]->state();
	}

	// Report state.
	EmergencyStopButton::state = state != Button::RUN;
}

