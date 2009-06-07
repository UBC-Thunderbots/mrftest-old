#include "datapool/EmergencyStopButton.h"
#include "Log/Log.h"
#include "test/Joystick.h"
#include "UI/ControlPanel.h"
#include "XBee/XBee.h"

#include <algorithm>
#include <vector>
#include <string>
#include <map>
#include <cstring>
#include <cmath>
#include <gtkmm/accelkey.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/comboboxtext.h>
#include <gtkmm/frame.h>
#include <gtkmm/label.h>
#include <gtkmm/main.h>
#include <gtkmm/notebook.h>
#include <gtkmm/radiobuttongroup.h>
#include <gtkmm/radiomenuitem.h>
#include <gtkmm/scale.h>
#include <gtkmm/window.h>
#include <time.h>

namespace {
	std::vector<std::tr1::shared_ptr<Joystick> > joysticks;

	template<typename T, T min, T max>
	class TypedScale : public Gtk::HScale {
	public:
		TypedScale(T &value) : Gtk::HScale(min, max, 1), value(value) {
			set_value(value);
			get_adjustment()->set_page_size(0);
		}

	protected:
		virtual void on_value_changed() {
			value = static_cast<T>(get_value() + 0.1);
		}

	private:
		T &value;
	};

	class KickButton : public Gtk::Button {
	public:
		KickButton(unsigned char &value) : Gtk::Button("Fire"), level(0), value(value) {
		}

		unsigned char level;

	protected:
		virtual void on_clicked() {
			value = level;
		}

	private:
		unsigned char &value;
	};

	class InputDevice : public Gtk::Frame {
	public:
		InputDevice() : Gtk::Frame("Controls") {
		}

		virtual void update() = 0;
		virtual void zero() = 0;
	};

	class Scales : public InputDevice {
	public:
		Scales(unsigned int id) : id(id), hBox(false, 5), vBox1(true, 0), vBox2(true, 0), vxLabel("Vx:"), vyLabel("Vy:"), vthetaLabel("Vtheta:"), dribbleLabel("Dribble:"), kickLabel("Kick:"), extraLabel("Extra:"), kickFire(XBee::out[id].kick), vx(XBee::out[id].vx), vy(XBee::out[id].vy), vt(XBee::out[id].vt), extra(XBee::out[id].extra), dribble(XBee::out[id].dribble), kickLevel(kickFire.level), kickBox(false, 0) {
			kickBox.pack_start(kickLevel, true, true);
			kickBox.pack_start(kickFire, false, false);

			vBox1.pack_start(vxLabel, false, false);
			vBox1.pack_start(vyLabel, false, false);
			vBox1.pack_start(vthetaLabel, false, false);
			vBox1.pack_start(dribbleLabel, false, false);
			vBox1.pack_start(kickLabel, false, false);
			vBox1.pack_start(extraLabel, false, false);

			vBox2.pack_start(vx, true, true);
			vBox2.pack_start(vy, true, true);
			vBox2.pack_start(vt, true, true);
			vBox2.pack_start(dribble, true, true);
			vBox2.pack_start(kickBox, true, true);
			vBox2.pack_start(extra, true, true);

			hBox.pack_start(vBox1, false, false);
			hBox.pack_start(vBox2, true, true);

			add(hBox);
		}

		virtual void zero() {
			vx.set_value(0);
			vy.set_value(0);
			vt.set_value(0);
		}

		virtual void update() {
		}

	private:
		const unsigned int id;
		Gtk::HBox hBox;
		Gtk::VBox vBox1, vBox2;
		Gtk::Label vxLabel, vyLabel, vthetaLabel, dribbleLabel, kickLabel, extraLabel;
		KickButton kickFire;
		TypedScale<char, -127, 127> vx, vy, vt, extra;
		TypedScale<unsigned char, 0, 255> dribble, kickLevel;
		Gtk::HBox kickBox;
	};

	class JoystickInput : public InputDevice {
	public:
		JoystickInput(unsigned int id, std::tr1::shared_ptr<Joystick> joystick) : id(id), joystick(joystick), label("Joystick active"), kicking(false) {
			add(label);
		}

		virtual void zero() {
		}

		virtual void update() {
			XBee::out[id].vx = 0;//joystick->axes[Joystick::AXIS_RX] / 256;
			XBee::out[id].vy = curve(-joystick->axes[Joystick::AXIS_RY] / 32767.0) * 127;
			XBee::out[id].vt = curve(-joystick->axes[Joystick::AXIS_LX] / 32767.0) * 127;
			XBee::out[id].dribble = (joystick->axes[Joystick::AXIS_LT] + 32767) / 256;
			XBee::out[id].reboot = joystick->buttons[Joystick::BTN_START] ? 255 : 0;
			if (joystick->buttons[Joystick::BTN_A] && !kicking) {
				XBee::out[id].kick = (joystick->axes[Joystick::AXIS_RT] + 32767) / 256;
				kicking = true;
			} else if (!joystick->buttons[Joystick::BTN_A] && kicking) {
				kicking = false;
			}
		}

	private:
		const unsigned int id;
		std::tr1::shared_ptr<Joystick> joystick;
		Gtk::Label label;
		bool kicking;

		static double curve(double x) {
			return std::fabs(x) * std::fabs(x) * (x < 0 ? -1 : 1);
		}
	};

	class RobotTab : public Gtk::VBox {
	public:
		RobotTab(unsigned int id) : Gtk::VBox(false, 1), id(id), input(new Scales(id)) {
			inputDeviceSelector.append_text("Sliders");
			for (unsigned int i = 0; i < joysticks.size(); i++)
				inputDeviceSelector.append_text(joysticks[i]->name());
			inputDeviceSelector.set_active_text("Sliders");
			inputDeviceSelector.signal_changed().connect(sigc::mem_fun(this, &RobotTab::inputDeviceChanged));

			pack_start(inputDeviceSelector, false, false);
			pack_start(*input, true, true);
		}

		void inputDeviceChanged() {
			if (input)
				remove(*input);
			const std::string &cur = inputDeviceSelector.get_active_text();
			if (cur == "Sliders") {
				input.reset(new Scales(id));
			} else {
				for (unsigned int i = 0; i < joysticks.size(); i++)
					if (joysticks[i]->name() == cur) {
						Log::log(Log::LEVEL_INFO, "test") << "Using joystick " << i << '\n';
						input.reset(new JoystickInput(id, joysticks[i]));
					}
			}
			pack_start(*input, true, true);
			input->show_all();
		}

		void zero() {
			input->zero();
		}

		void update() {
			input->update();
		}

	private:
		const unsigned int id;
		Gtk::ComboBoxText inputDeviceSelector;
		std::tr1::shared_ptr<InputDevice> input;
	};

	class RobotTabs : public Gtk::Notebook {
	public:
		RobotTabs() : robot0(0), robot1(1), robot2(2), robot3(3), robot4(4) {
			append_page(robot0, "Robot 0");
			append_page(robot1, "Robot 1");
			append_page(robot2, "Robot 2");
			append_page(robot3, "Robot 3");
			append_page(robot4, "Robot 4");
		}

		void zero() {
			robot0.zero();
			robot1.zero();
			robot2.zero();
			robot3.zero();
			robot4.zero();
		}

		void update() {
			robot0.update();
			robot1.update();
			robot2.update();
			robot3.update();
			robot4.update();
		}

	private:
		RobotTab robot0, robot1, robot2, robot3, robot4;
	};

	class MainContainer : public Gtk::VBox {
	public:
		MainContainer() {
			pack_start(rt, true, true);
			pack_start(cp, false, false);
		}

		void update() {
			cp.update();
			rt.update();
		}

		void zero() {
			rt.zero();
		}

	private:
		RobotTabs rt;
		ControlPanel cp;
	};

	class MainWindow : public Gtk::Window {
	public:
		MainWindow() {
			set_title("Thunderbots Tester");
			add(mc);
		}

		bool update() {
			for (unsigned int i = 0; i < joysticks.size(); i++)
				joysticks[i]->update();
			EmergencyStopButton::update();
			mc.update();
			XBee::update();
			return true;
		}

		void zero() {
			mc.zero();
		}

	protected:
		virtual bool on_key_press_event(GdkEventKey *evt) {
			if (evt->keyval == GDK_z) {
				zero();
				return true;
			} else {
				return Gtk::Window::on_key_press_event(evt);
			}
		}

	private:
		MainContainer mc;
	};
}

int main(int argc, char **argv) {
	Gtk::Main kit(argc, argv);
	if (argc > 1 && strcmp(argv[1], "-d") == 0) {
		Log::setLevel(Log::LEVEL_DEBUG);
	}
	const std::vector<std::string> &stickNames = Joystick::list();
	for (unsigned int i = 0; i < stickNames.size(); i++) {
		try {
			std::tr1::shared_ptr<Joystick> ptr(new Joystick(stickNames[i]));
			joysticks.push_back(ptr);
		} catch (...) {
			// Swallow.
		}
	}
	XBee::init();
	for (unsigned int i = 0; i < Team::SIZE; i++)
		XBee::out[i].extra = 0;
	EmergencyStopButton::init();
	MainWindow mw;
	sigc::connection timer = Glib::signal_timeout().connect(sigc::bind(sigc::mem_fun(&MainWindow::update), &mw), 33);
	mw.show_all();
	kit.run(mw);
	timer.disconnect();
	return 0;
}

