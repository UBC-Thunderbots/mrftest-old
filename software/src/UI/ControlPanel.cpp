#include "UI/ControlPanel.h"
#include "datapool/EmergencyStopButton.h"
#include "datapool/LargeBatt.h"
#include "datapool/SmallBatt.h"
#include "XBee/XBee.h"

#include <tr1/memory>
#include <gtkmm/button.h>
#include <gtkmm/drawingarea.h>
#include <gtkmm/label.h>
#include <gtkmm/progressbar.h>
#include <gtkmm/table.h>
#include <gtkmm/togglebutton.h>

namespace {
	class CommStatusLight : public Gtk::DrawingArea {
	public:
		CommStatusLight(unsigned int id) : id(id), old(XBee::STATUS_OK) {
			set_size_request(32, 32);
		}

		void update() {
			if (XBee::commStatus[id] == old)
				return;
			Glib::RefPtr<Gdk::Window> win(get_window());
			const Gdk::Rectangle &r(get_allocation());
			if (win)
				win->invalidate_rect(Gdk::Rectangle(0, 0, r.get_width(), r.get_height()), false);
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
			if (XBee::commStatus[id] == XBee::STATUS_OK)
				ctx->set_source_rgb(0, 1, 0);
			else if (XBee::commStatus[id] == XBee::STATUS_NO_RECV)
				ctx->set_source_rgb(1, 1, 0);
			else
				ctx->set_source_rgb(1, 0, 0);
			ctx->arc(w / 2, h / 2, std::min(w, h) / 2, 0, 2 * M_PI);
			ctx->fill();

			old = XBee::commStatus[id];

			return true;
		}

	private:
		const unsigned int id;
		XBee::CommStatus old;
	};

	class Voltage : public Gtk::ProgressBar {
	public:
		Voltage(const unsigned char *data, const double *lut) : data(data), lut(lut), old(UINT_MAX) {
			set_text("---");
		}

		void update() {
			unsigned int level = data[0] * 256 + data[1];
			if (level == old)
				return;
			if (level > 1023) {
				set_text("---");
			} else {
				double percent = lut[level];
				std::ostringstream oss;
				oss << static_cast<unsigned int>(percent * 100 + 0.1) << "% [" << level << ']';
				set_text(oss.str());
				set_fraction(percent);
			}
			old = level;
		}

	private:
		const unsigned char *data;
		const double *lut;
		unsigned int old;
	};

	class RunSwitch : public Gtk::ToggleButton {
	public:
		RunSwitch(unsigned int id) : Gtk::ToggleButton("Run"), id(id), old(true) {
			set_active(false);
			set_sensitive(false);
			XBee::out[id].emergency = 0xFF;
		}

		void update() {
			if (old == EmergencyStopButton::state)
				return;
			if (EmergencyStopButton::state) {
				set_active(false);
				set_sensitive(false);
			} else {
				set_sensitive(true);
			}

			XBee::out[id].emergency = get_active() ? 0 : 0xFF;
			old = EmergencyStopButton::state;
		}

	protected:
		virtual void on_toggled() {
			XBee::out[id].emergency = get_active() ? 0 : 0xFF;
		}

	private:
		unsigned int id;
		bool old;
	};

	class RebootButton : public Gtk::Button {
	public:
		RebootButton(unsigned int id) : Gtk::Button("Reboot"), id(id), pending(false) {
			XBee::out[id].reboot = 0;
		}

		void update() {
			XBee::out[id].reboot = pending ? 0xFF : 0;
			pending = false;
		}

	protected:
		virtual void on_clicked() {
			pending = true;
		}

	private:
		const unsigned int id;
		bool pending;
	};

	class RobotControls {
	public:
		RobotControls(unsigned int id, Gtk::Table &tbl) : commStatusLight(id), runSwitch(id), rebootButton(id), greenVoltage(XBee::in[id].vGreen, SmallBattDisCurve), motorVoltage(XBee::in[id].vMotor, LargeBattDisCurve) {
			std::ostringstream oss;
			oss << id;
			label.set_text(oss.str());

			tbl.attach(label,           id + 1, id + 2, 0, 1);
			tbl.attach(commStatusLight, id + 1, id + 2, 1, 2);
			tbl.attach(runSwitch,       id + 1, id + 2, 2, 3);
			tbl.attach(rebootButton,    id + 1, id + 2, 3, 4);
			tbl.attach(greenVoltage,    id + 1, id + 2, 4, 5);
			tbl.attach(motorVoltage,    id + 1, id + 2, 5, 6);
		}

		void update() {
			commStatusLight.update();
			runSwitch.update();
			rebootButton.update();
			greenVoltage.update();
			motorVoltage.update();
		}

	private:
		Gtk::Label label;
		CommStatusLight commStatusLight;
		RunSwitch runSwitch;
		RebootButton rebootButton;
		Voltage greenVoltage;
		Voltage motorVoltage;
	};

	class RobotControlArray {
	public:
		RobotControlArray(Gtk::Table &tbl) : r0(0, tbl), r1(1, tbl), r2(2, tbl), r3(3, tbl), r4(4, tbl) {
		}

		void update() {
			r0.update();
			r1.update();
			r2.update();
			r3.update();
			r4.update();
		}

	private:
		RobotControls r0, r1, r2, r3, r4;
	};
}

class ControlPanelImpl {
public:
	ControlPanelImpl(Gtk::Table &tbl) : controlArray(tbl) {
	}

	Gtk::Label robotIdLabel, commStatusLabel, runLabel, rebootLabel, greenLabel, motorLabel;
	RobotControlArray controlArray;
};

ControlPanel::ControlPanel() : Gtk::Table(6, 6, false), impl(new ControlPanelImpl(*this)) {
	impl->robotIdLabel.set_text("Robot:");
	impl->commStatusLabel.set_text("Comm Status:");
	impl->runLabel.set_text("Run Switch:");
	impl->rebootLabel.set_text("Reboot:");
	impl->greenLabel.set_text("Green Battery:");
	impl->motorLabel.set_text("Motor Battery:");

	attach(impl->robotIdLabel,    0, 1, 0, 1);
	attach(impl->commStatusLabel, 0, 1, 1, 2);
	attach(impl->runLabel,        0, 1, 2, 3);
	attach(impl->rebootLabel,     0, 1, 3, 4);
	attach(impl->greenLabel,      0, 1, 4, 5);
	attach(impl->motorLabel,      0, 1, 5, 6);
}

void ControlPanel::update() {
	impl->controlArray.update();
}

