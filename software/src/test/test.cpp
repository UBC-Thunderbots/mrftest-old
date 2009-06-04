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
	class SignedCharScale : public Gtk::HScale {
	public:
		SignedCharScale() : Gtk::HScale(-127, 127, 1) {
			set_value(0);
			get_adjustment()->set_page_size(0);
		}

		char value() const {
			return static_cast<char>(get_value() + 0.1);
		}

		void zero() {
			set_value(0);
		}
	};

	class UnsignedCharScale : public Gtk::HScale {
	public:
		UnsignedCharScale() : Gtk::HScale(0, 255, 1) {
			get_adjustment()->set_page_size(0);
		}

		unsigned char value() const {
			return static_cast<unsigned char>(get_value() + 0.1);
		}
	};

	class KickButton : public Gtk::Button {
	public:
		KickButton() : Gtk::Button("Fire"), pending(false) {
		}

		virtual void on_clicked() {
			pending = true;
		}

		bool pending;
	};

	class Scales : public Gtk::Frame {
	public:
		Scales(unsigned int id) : Gtk::Frame("Controls"), id(id), hBox(false, 5), vBox1(true, 0), vBox2(true, 0), vxLabel("Vx:"), vyLabel("Vy:"), vthetaLabel("Vtheta:"), dribbleLabel("Dribble:"), kickLabel("Kick:"), extra1Label("Extra #1:"), extra2Label("Extra #2:"), kickBox(false, 0) {
			kickBox.pack_start(kickLevel, true, true);
			kickBox.pack_start(kickFire, false, false);

			vBox1.pack_start(vxLabel, false, false);
			vBox1.pack_start(vyLabel, false, false);
			vBox1.pack_start(vthetaLabel, false, false);
			vBox1.pack_start(dribbleLabel, false, false);
			vBox1.pack_start(kickLabel, false, false);
			vBox1.pack_start(extra1Label, false, false);
			vBox1.pack_start(extra2Label, false, false);

			vBox2.pack_start(vx, true, true);
			vBox2.pack_start(vy, true, true);
			vBox2.pack_start(vtheta, true, true);
			vBox2.pack_start(dribble, true, true);
			vBox2.pack_start(kickBox, true, true);
			vBox2.pack_start(extra1, true, true);
			vBox2.pack_start(extra2, true, true);

			hBox.pack_start(vBox1, false, false);
			hBox.pack_start(vBox2, true, true);

			add(hBox);
		}

		void update() {
			XBee::out[id].vx = vx.value();
			XBee::out[id].vy = vy.value();
			XBee::out[id].vtheta = vtheta.value();
			XBee::out[id].dribble = dribble.value();
			if (kickFire.pending) {
				XBee::out[id].kick = kickLevel.value();
				kickFire.pending = false;
			} else {
				XBee::out[id].kick = 0;
			}
			XBee::out[id].extra1 = extra1.value();
			XBee::out[id].extra2 = extra2.value();
		}

		void zero() {
			vx.zero();
			vy.zero();
			vtheta.zero();
		}

	private:
		const unsigned int id;
		Gtk::HBox hBox;
		Gtk::VBox vBox1, vBox2;
		Gtk::Label vxLabel, vyLabel, vthetaLabel, dribbleLabel, kickLabel, extra1Label, extra2Label;
		SignedCharScale vx, vy, vtheta, extra1, extra2;
		UnsignedCharScale dribble, kickLevel;
		KickButton kickFire;
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

		void update() {
			robot0.update();
			robot1.update();
			robot2.update();
			robot3.update();
			robot4.update();
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
			rt.update();
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
	EmergencyStopButton::init();
	MainWindow mw;
	sigc::connection timer = Glib::signal_timeout().connect(sigc::bind(sigc::mem_fun(&MainWindow::update), &mw), 33);
	mw.show_all();
	kit.run(mw);
	timer.disconnect();
	return 0;
}

