#ifndef TB_JOYSTICK_H
#define TB_JOYSTICK_H

#include <string>
#include <vector>

class Joystick {
public:
	static std::vector<std::string> list();

	Joystick(const std::string &filename);
	~Joystick();
	bool update();

	const std::string &name() const {
		return filename;
	}

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
	Joystick(const Joystick &copyref); // Inhibit copying.
	int fd;
	const std::string filename;
};

#endif

