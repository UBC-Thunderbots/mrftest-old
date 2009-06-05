#include "datapool/EmergencyStopButton.h"
#include "Log/Log.h"
#include "UI/ControlPanel.h"
#include "XBee/XBee.h"

#include <algorithm>
#include <cstring>
#include <gtkmm/accelkey.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/frame.h>
#include <gtkmm/label.h>
#include <gtkmm/main.h>
#include <gtkmm/notebook.h>
#include <gtkmm/scale.h>
#include <gtkmm/window.h>
#include <time.h>

namespace {
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

	class Scales : public Gtk::Frame {
	public:
		Scales(unsigned int id) : Gtk::Frame("Controls"), id(id), hBox(false, 5), vBox1(true, 0), vBox2(true, 0), vxLabel("Vx:"), vyLabel("Vy:"), vthetaLabel("Vtheta:"), dribbleLabel("Dribble:"), kickLabel("Kick:"), extraLabel("Extra:"), kickFire(XBee::out[id].kick), vx(XBee::out[id].vx), vy(XBee::out[id].vy), vt(XBee::out[id].vt), extra(XBee::out[id].extra), dribble(XBee::out[id].dribble), kickLevel(kickFire.level), kickBox(false, 0) {
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

		void zero() {
			vx.set_value(0);
			vy.set_value(0);
			vt.set_value(0);
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

	private:
		Scales robot0, robot1, robot2, robot3, robot4;
	};

	class MainContainer : public Gtk::VBox {
	public:
		MainContainer() {
			pack_start(rt, true, true);
			pack_start(cp, false, false);
		}

		void update() {
			cp.update();
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

