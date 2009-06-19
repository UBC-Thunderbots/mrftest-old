#ifndef TEST_JOYSTICK_H
#define TEST_JOYSTICK_H

#include <string>
#include <vector>

#include <sigc++/sigc++.h>
#include <glibmm.h>

class Joystick {
public:
	sigc::signal<void> &signal_changed();
	const std::string &name() const;
	static const std::vector<Glib::RefPtr<Joystick> > all();
	static Glib::RefPtr<Joystick> create(const std::string &device);
	void reference();
	void unreference();

	enum Button {
		BTN_A,
		BTN_B,
		BTN_X,
		BTN_Y,
		BTN_L,
		BTN_R,
		BTN_START,
		BTN_XBOX,
		BTN_LSTICK,
		BTN_RSTICK,
		BTN_UP,
		BTN_DOWN,
		BTN_LEFT,
		BTN_RIGHT,
		BTN_BACK,
		NUM_BTNS
	};

	enum Axis {
		AXIS_LX,
		AXIS_LY,
		AXIS_LT,
		AXIS_RX,
		AXIS_RY,
		AXIS_RT,
		NUM_AXES
	};

	bool buttons[NUM_BTNS];
	short axes[NUM_AXES];

private:
	int fd;
	const std::string filename;
	sigc::signal<void> sig_changed;
	sigc::connection ioConnection;
	unsigned int refs;

	Joystick(int fd, const std::string &device);
	Joystick(const Joystick &copyref); // Inhibit copying.
	~Joystick();
	bool onIO(Glib::IOCondition cond);
};

#endif

