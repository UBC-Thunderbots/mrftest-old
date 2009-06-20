#include "datapool/Config.h"
#include "datapool/HWRunSwitch.h"
#include "Log/Log.h"
#include "Test/Joystick.h"
#include "UI/ControlPanel.h"
#include "XBee/XBeeBot.h"

#include <iostream>
#include <vector>
#include <string>
#include <cmath>

#include <getopt.h>
#include <gtkmm.h>

namespace {
	class BotScale : public Gtk::HScale {
	public:
		BotScale(double min, double max, Glib::RefPtr<XBeeBot> bot, void (XBeeBot::*setter)(double)) : Gtk::HScale(min, max + 0.01, 0.01), bot(bot), setter(setter) {
			set_value(0);
		}

	protected:
		virtual void on_value_changed() {
			Gtk::HScale::on_value_changed();

			// This is disgusting, but Glib::RefPtr<> doesn't overload operator->* so we have no choice.
			XBeeBot *b = bot.operator->();
			(b->*setter)(get_value());
		}

	private:
		Glib::RefPtr<XBeeBot> bot;
		void (XBeeBot::*setter)(double);
	};

	class KickButton : public Gtk::Button {
	public:
		KickButton(Glib::RefPtr<XBeeBot> bot, Gtk::Range &level) : Gtk::Button("Fire"), bot(bot), level(level) {
		}

	protected:
		virtual void on_clicked() {
			Gtk::Button::on_clicked();
			bot->kick(level.get_value());
			Log::log(Log::LEVEL_DEBUG, "Kick") << "Kicking at strength " << level.get_value() << '\n';
		}

	private:
		Glib::RefPtr<XBeeBot> bot;
		Gtk::Range &level;
	};

	class InputDevice : public Gtk::Frame {
	public:
		InputDevice() : Gtk::Frame("Controls") {
		}

		virtual void zero() = 0;
	};

	class Scales : public InputDevice {
	public:
		Scales(Glib::RefPtr<XBeeBot> bot) : hBox(false, 5), vBox1(true, 0), vBox2(true, 0), vxLabel("Vx:"), vyLabel("Vy:"), vthetaLabel("Vtheta:"), dribbleLabel("Dribble:"), kickLabel("Kick:"), vx(-1, 1, bot, &XBeeBot::vx), vy(-1, 1, bot, &XBeeBot::vy), vt(-1, 1, bot, &XBeeBot::vt), dribble(0, 1, bot, &XBeeBot::dribbler), kickLevel(0, 1.01, 0.01), kickFire(bot, kickLevel), kickBox(false, 0) {
			kickBox.pack_start(kickLevel, true, true);
			kickBox.pack_start(kickFire, false, false);

			vBox1.pack_start(vxLabel, false, false);
			vBox1.pack_start(vyLabel, false, false);
			vBox1.pack_start(vthetaLabel, false, false);
			vBox1.pack_start(dribbleLabel, false, false);
			vBox1.pack_start(kickLabel, false, false);

			vBox2.pack_start(vx, true, true);
			vBox2.pack_start(vy, true, true);
			vBox2.pack_start(vt, true, true);
			vBox2.pack_start(dribble, true, true);
			vBox2.pack_start(kickBox, true, true);

			hBox.pack_start(vBox1, false, false);
			hBox.pack_start(vBox2, true, true);

			add(hBox);
		}

		virtual void zero() {
			vx.set_value(0);
			vy.set_value(0);
			vt.set_value(0);
		}

	private:
		Gtk::HBox hBox;
		Gtk::VBox vBox1, vBox2;
		Gtk::Label vxLabel, vyLabel, vthetaLabel, dribbleLabel, kickLabel;
		BotScale vx, vy, vt, dribble;
		Gtk::HScale kickLevel;
		KickButton kickFire;
		Gtk::HBox kickBox;
	};

	class JoystickInput : public InputDevice, private virtual sigc::trackable {
	public:
		JoystickInput(Glib::RefPtr<XBeeBot> bot, Glib::RefPtr<Joystick> joystick) : bot(bot), joystick(joystick), label("Joystick active"), kicking(false) {
			add(label);
			joystick->signal_changed().connect(sigc::mem_fun(*this, &JoystickInput::onChange));
		}

		virtual void zero() {
		}

	private:
		Glib::RefPtr<XBeeBot> bot;
		Glib::RefPtr<Joystick> joystick;
		Gtk::Label label;
		bool kicking;

		void onChange() {
			bot->vx(curve(+joystick->axes[Joystick::AXIS_RX] / 32767.0));
			bot->vy(curve(-joystick->axes[Joystick::AXIS_RY] / 32767.0));
			bot->vt(curve(-joystick->axes[Joystick::AXIS_LX] / 32767.0));
			bot->dribbler((joystick->axes[Joystick::AXIS_LT] + 32767) / 65535.0);
			if (joystick->buttons[Joystick::BTN_A] && !kicking) {
				bot->kick((joystick->axes[Joystick::AXIS_RT] + 32767) / 65535.0);
				kicking = true;
			} else if (!joystick->buttons[Joystick::BTN_A] && kicking) {
				kicking = false;
			}
			if (joystick->buttons[Joystick::BTN_START])
				bot->reboot();
		}

		static double curve(double x) {
			return std::fabs(x) * std::fabs(x) * (x < 0 ? -1 : 1);
		}
	};

	class RobotTab : public Gtk::VBox {
	public:
		RobotTab(Glib::RefPtr<XBeeBot> bot) : Gtk::VBox(false, 1), bot(bot) {
			inputDeviceSelector.append_text("Sliders");
			for (unsigned int i = 0; i < Joystick::all().size(); i++)
				inputDeviceSelector.append_text(Joystick::all()[i]->name());
			inputDeviceSelector.set_active_text("Sliders");
			inputDeviceSelector.signal_changed().connect(sigc::mem_fun(this, &RobotTab::inputDeviceChanged));

			inputDevice = Gtk::manage(new Scales(bot));

			pack_start(inputDeviceSelector, false, false);
			pack_start(*inputDevice, true, true);
		}

		void inputDeviceChanged() {
			remove(*inputDevice);
			inputDevice = 0;

			const std::string &cur = inputDeviceSelector.get_active_text();
			if (cur == "Sliders")
				inputDevice = Gtk::manage(new Scales(bot));
			else
				for (unsigned int i = 0; i < Joystick::all().size(); i++)
					if (Joystick::all()[i]->name() == cur)
						inputDevice = Gtk::manage(new JoystickInput(bot, Joystick::all()[i]));

			pack_start(*inputDevice, true, true);
			inputDevice->show_all();
		}

		void zero() {
			inputDevice->zero();
		}

	private:
		Glib::RefPtr<XBeeBot> bot;
		Gtk::ComboBoxText inputDeviceSelector;
		InputDevice *inputDevice;
	};

	class RobotTabs : public Gtk::Notebook {
	public:
		RobotTabs() {
			XBeeBotSet &set = XBeeBotSet::instance();
			for (unsigned int i = 0; i < set.size(); i++) {
				Glib::RefPtr<RobotTab> tab(new RobotTab(set[i]));
				tabs.push_back(tab);
				std::ostringstream oss;
				oss << "Robot " << i;
				// Ugly hack: Glib::RefPtr<> does not allow getting its value!
				append_page(*tab.operator->(), oss.str());
			}
		}

		~RobotTabs() {
			pages().clear();
			tabs.clear();
		}

		void zero() {
			for (unsigned int i = 0; i < tabs.size(); i++)
				tabs[i]->zero();
		}

	private:
		std::vector<Glib::RefPtr<RobotTab> > tabs;
	};

	class MainWindow : public Gtk::Window {
	public:
		MainWindow() {
			set_title("Thunderbots Tester");
			add(rt);
			show_all();
		}

		void zero() {
			rt.zero();
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

		virtual bool on_delete_event(GdkEventAny *evt) {
			Gtk::Window::on_delete_event(evt);
			Gtk::Main::quit();
			return true;
		}

	private:
		RobotTabs rt;
	};

	void usage(const char *appname) {
		std::cerr <<
			"Usage:\n" <<
			appname << " [-d|--debug]\n"
			"-d: enable debug mode\n";
	}
}

int main(int argc, char **argv) {
	// Create the GTK main object.
	Gtk::Main kit(argc, argv);

	// Parse remaining command-line options.
	static const char SHORT_OPTIONS[] = "d";
	static const struct option LONG_OPTIONS[] = {
		{"debug", false, 0, 'd'},
		{0, 0, 0, 0}
	};
	int ch;
	do {
		ch = getopt_long(argc, argv, SHORT_OPTIONS, LONG_OPTIONS, 0);
		switch (ch) {
			case 'd':
				Log::setLevel(Log::LEVEL_DEBUG);
				break;

			case '?':
				usage(argv[0]);
				return 1;
		}
	} while (ch != -1);
	if (optind != argc) {
		usage(argv[0]);
		return 1;
	}

	// Load the config file.
	Config config;

	// Initialize the hardware run switch.
	HWRunSwitch hwrs;

	// Initialize the radio.
	XBeeBotSet xbbs;

	// Create the control panel and main window.
	ControlPanel cp;
	MainWindow mw;

	// Run the application.
	Gtk::Main::run();

	return 0;
}

