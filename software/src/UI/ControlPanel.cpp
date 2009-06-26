#include "datapool/HWRunSwitch.h"
#include "UI/ControlPanel.h"
#include "XBee/XBeeBot.h"

#include <iostream>
#include <cassert>
#include <gtkmm.h>

#define GREEN_BATTERY_MAX_VOLTAGE       12.50
#define GREEN_BATTERY_MIN_VOLTAGE        9.00
#define GREEN_BATTERY_WARNING_VOLTAGE    9.90
#define GREEN_BATTERY_ZERO_VOLTAGE       2.00

#define MOTOR_BATTERY_MAX_VOLTAGE       16.75
#define MOTOR_BATTERY_MIN_VOLTAGE       14.40
#define MOTOR_BATTERY_WARNING_VOLTAGE   14.60
#define MOTOR_BATTERY_ZERO_VOLTAGE       2.00

namespace {
	class Beeper : public virtual sigc::trackable {
	public:
		Beeper() {
			Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &Beeper::onTimeout), 1);
		}

	private:
		bool onTimeout() {
			bool shouldBeep = false;
			XBeeBotSet &bots = XBeeBotSet::instance();
			for (unsigned int i = 0; i < bots.size(); i++) {
				if (bots[i]->property_commStatus() == XBeeBot::STATUS_NO_ACK)
					continue;
				double green = bots[i]->property_greenVoltage();
				double motor = bots[i]->property_motorVoltage();
				if (green > GREEN_BATTERY_ZERO_VOLTAGE && green < GREEN_BATTERY_WARNING_VOLTAGE)
					shouldBeep = true;
				if (motor > MOTOR_BATTERY_ZERO_VOLTAGE && motor < MOTOR_BATTERY_WARNING_VOLTAGE)
					shouldBeep = true;
			}
			if (shouldBeep) {
				std::cerr << '\a';
				std::cerr.flush();
			}
			return true;
		}
	};

	class CommStatusLight : public Gtk::DrawingArea, private virtual sigc::trackable {
	public:
		CommStatusLight(Glib::RefPtr<XBeeBot> bot) : bot(bot) {
			set_size_request(32, 32);
			bot->property_commStatus().signal_changed().connect(sigc::mem_fun(*this, &CommStatusLight::onChange));
		}

	protected:
		virtual bool on_expose_event(GdkEventExpose *evt) {
			Glib::RefPtr<Gdk::Window> win(get_window());
			if (!win)
				return true;

			Cairo::RefPtr<Cairo::Context> ctx(win->create_cairo_context());
			if (evt) {
				ctx->rectangle(evt->area.x, evt->area.y, evt->area.width, evt->area.height);
				ctx->clip();
			}

			const Gdk::Rectangle &r(get_allocation());
			double w = r.get_width();
			double h = r.get_height();
			switch (bot->property_commStatus()) {
				case XBeeBot::STATUS_OK:
					ctx->set_source_rgb(0, 1, 0);
					break;

				case XBeeBot::STATUS_NO_RECV:
					ctx->set_source_rgb(1, 1, 0);
					break;

				case XBeeBot::STATUS_NO_ACK:
					ctx->set_source_rgb(1, 0, 0);
					break;
			}
			ctx->arc(w / 2, h / 2, std::min(w, h) / 2, 0, 2 * M_PI);
			ctx->fill();

			return true;
		}

	private:
		Glib::RefPtr<XBeeBot> bot;

		void onChange() {
			Glib::RefPtr<Gdk::Window> win(get_window());
			const Gdk::Rectangle &r(get_allocation());
			if (win)
				win->invalidate_rect(Gdk::Rectangle(0, 0, r.get_width(), r.get_height()), false);
		}
	};

	class Voltage : public Gtk::ProgressBar, private virtual sigc::connection {
	public:
		Voltage(Glib::PropertyProxy<double> voltage, double min, double max, double warn) : voltage(voltage), min(min), max(max), warn(warn) {
			voltage.signal_changed().connect(sigc::mem_fun(*this, &Voltage::onChange));
		}

	private:
		Glib::PropertyProxy<double> voltage;
		const double min, max, warn;

		void onChange() {
			std::ostringstream oss;
			oss.setf(std::ios_base::fixed, std::ios_base::floatfield);
			oss.precision(2);
			oss << voltage << 'V';
			set_text(oss.str());
			if (voltage < min)
				set_fraction(0);
			else if (voltage > max)
				set_fraction(1);
			else
				set_fraction((voltage - min) / (max - min));

			Gdk::Color clr(voltage <= warn ? "red" : "green");
			modify_bg(Gtk::STATE_PRELIGHT, clr);
		}
	};

	class RunSwitch : public Gtk::ToggleButton, public virtual sigc::trackable {
	public:
		RunSwitch(Glib::RefPtr<XBeeBot> bot) : Gtk::ToggleButton("Run"), bot(bot) {
			HWRunSwitch::instance().property_state().signal_changed().connect(sigc::mem_fun(*this, &RunSwitch::onChange));
			set_sensitive(HWRunSwitch::instance().property_state());
		}

	protected:
		virtual void on_toggled() {
			bot->run(get_active());
		}

	private:
		Glib::RefPtr<XBeeBot> bot;

		void onChange() {
			if (HWRunSwitch::instance().property_state()) {
				set_sensitive(true);
			} else {
				set_active(false);
				set_sensitive(false);
			}
		}
	};

	class RebootButton : public Gtk::Button {
	public:
		RebootButton(Glib::RefPtr<XBeeBot> bot) : Gtk::Button("Reboot"), bot(bot) {
		}

	protected:
		virtual void on_clicked() {
			bot->reboot();
		}

	private:
		Glib::RefPtr<XBeeBot> bot;
	};

	class FirmwareVersion : public Gtk::Entry, public virtual sigc::trackable {
	public:
		FirmwareVersion(Glib::RefPtr<XBeeBot> bot) : bot(bot) {
			set_sensitive(false);
			bot->property_firmwareVersion().signal_changed().connect(sigc::mem_fun(*this, &FirmwareVersion::onChange));
		}

	private:
		Glib::RefPtr<XBeeBot> bot;

		void onChange() {
			unsigned int fwv = bot->property_firmwareVersion();
			std::ostringstream oss;
			oss << fwv;
			set_text(oss.str());
		}
	};

	class HasGyro : public Gtk::CheckButton, public virtual sigc::trackable {
	public:
		HasGyro(Glib::RefPtr<XBeeBot> bot) : bot(bot) {
			set_sensitive(false);
			bot->property_hasGyro().signal_changed().connect(sigc::mem_fun(*this, &HasGyro::onChange));
		}

	private:
		Glib::RefPtr<XBeeBot> bot;

		void onChange() {
			set_active(bot->property_hasGyro());
		}
	};

	class ControlPanelWindow : public Gtk::Window {
	public:
		ControlPanelWindow() : tbl(8, XBeeBotSet::instance().size() + 1, false) {
			set_title("Thunderbots Control Panel");

			robotIdLabel.set_text("Robot:");
			commStatusLabel.set_text("Comm Status:");
			runLabel.set_text("Run Switch:");
			rebootLabel.set_text("Reboot:");
			greenLabel.set_text("Green Battery:");
			motorLabel.set_text("Motor Battery:");
			firmwareLabel.set_text("Firmware:");
			gyroLabel.set_text("Has Gyro:");

			tbl.attach(robotIdLabel,    0, 1, 0, 1);
			tbl.attach(commStatusLabel, 0, 1, 1, 2);
			tbl.attach(runLabel,        0, 1, 2, 3);
			tbl.attach(rebootLabel,     0, 1, 3, 4);
			tbl.attach(greenLabel,      0, 1, 4, 5);
			tbl.attach(motorLabel,      0, 1, 5, 6);
			tbl.attach(firmwareLabel,   0, 1, 6, 7);
			tbl.attach(gyroLabel,       0, 1, 7, 8);

			for (unsigned int i = 0; i < XBeeBotSet::instance().size(); i++) {
				Glib::RefPtr<XBeeBot> bot = XBeeBotSet::instance()[i];

				std::ostringstream oss;
				oss << i;

				tbl.attach(*Gtk::manage(new Gtk::Label(oss.str())), i + 1, i + 2, 0, 1);
				tbl.attach(*Gtk::manage(new CommStatusLight(bot)), i + 1, i + 2, 1, 2);
				tbl.attach(*Gtk::manage(new RunSwitch(bot)), i + 1, i + 2, 2, 3);
				tbl.attach(*Gtk::manage(new RebootButton(bot)), i + 1, i + 2, 3, 4);
				tbl.attach(*Gtk::manage(new Voltage(bot->property_greenVoltage(), GREEN_BATTERY_MIN_VOLTAGE, GREEN_BATTERY_MAX_VOLTAGE, GREEN_BATTERY_WARNING_VOLTAGE)), i + 1, i + 2, 4, 5);
				tbl.attach(*Gtk::manage(new Voltage(bot->property_motorVoltage(), MOTOR_BATTERY_MIN_VOLTAGE, MOTOR_BATTERY_MAX_VOLTAGE, MOTOR_BATTERY_WARNING_VOLTAGE)), i + 1, i + 2, 5, 6);
				tbl.attach(*Gtk::manage(new FirmwareVersion(bot)), i + 1, i + 2, 6, 7);
				tbl.attach(*Gtk::manage(new HasGyro(bot)), i + 1, i + 2, 7, 8);
			}

			add(tbl);
		}

		~ControlPanelWindow() {
			remove();
		}

	protected:
		virtual bool on_delete_event(GdkEventAny *evt) {
			Gtk::Window::on_delete_event(evt);
			Gtk::Main::quit();
			return true;
		}

	private:
		Gtk::Table tbl;
		Gtk::Label robotIdLabel, commStatusLabel, runLabel, rebootLabel, greenLabel, motorLabel, firmwareLabel, gyroLabel;
	};
}

class ControlPanelImpl : private Noncopyable {
public:
	ControlPanelImpl() {
		cp.show_all();
	}

private:
	ControlPanelWindow cp;
	Beeper beeper;
};

ControlPanel::ControlPanel() : impl(new ControlPanelImpl) {
}

ControlPanel::~ControlPanel() {
	delete impl;
}

