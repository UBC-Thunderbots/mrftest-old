#include "mrftest/drive.h"
#include "util/algorithm.h"

namespace {
	void on_update_coast(Gtk::HScale(&)[4], MRFRobot &robot, bool) {
		robot.drive_coast();
	}

	void get_low_sensitivity_scale_factors_coast(double (&scale)[4]) {
		scale[0] = scale[1] = scale[2] = scale[3] = 0;
	}

	void on_update_brake(Gtk::HScale(&)[4], MRFRobot &robot, bool) {
		robot.drive_brake();
	}

	void get_low_sensitivity_scale_factors_brake(double (&scale)[4]) {
		scale[0] = scale[1] = scale[2] = scale[3] = 0;
	}

	void on_update_permotor(Gtk::HScale(&controls)[4], MRFRobot &robot, bool controlled) {
		int wheels[G_N_ELEMENTS(controls)];
		for (unsigned int i = 0; i < G_N_ELEMENTS(controls); ++i) {
			wheels[i] = clamp_symmetric(static_cast<int>(controls[i].get_value()), 1023);
		}
		robot.drive(wheels, controlled);
	}

	void get_low_sensitivity_scale_factors_permotor(double (&scale)[4]) {
		scale[0] = scale[1] = scale[2] = scale[3] = 0.1;
	}

	void on_update_matrix(Gtk::HScale(&controls)[4], MRFRobot &robot, bool controlled) {
		static const double matrix[4][3] = {
					{	-71.85,	46.66,	7.06},
					{	-60.58,	-60.58,	7.06},
					{	60.58,	-60.58,	7.06},
					{	71.85,	46.68,	7.06}
		};
		double input[G_N_ELEMENTS(matrix[0])];
		for (unsigned int i = 0; i < G_N_ELEMENTS(input); ++i) {
			input[i] = controls[i].get_value();
		}
		double output[G_N_ELEMENTS(matrix)] = { 0, 0, 0, 0 };
		for (unsigned int row = 0; row < G_N_ELEMENTS(output); ++row) {
			for (unsigned int col = 0; col < G_N_ELEMENTS(input); ++col) {
				output[row] += matrix[row][col] * input[col];
			}
		}
		int w[G_N_ELEMENTS(output)];
		for (unsigned int i = 0; i < G_N_ELEMENTS(w); ++i) {
			w[i] = clamp_symmetric(static_cast<int>(output[i]), 1023);
		}
		robot.drive(w, controlled);
	}

	void get_low_sensitivity_scale_factors_matrix(double (&scale)[4]) {
		scale[0] = 0.1;
		scale[1] = 0.1;
		scale[2] = 0.3;
		scale[3] = 0;
	}

	const struct {
		const char *name;
		unsigned int sensitive_mask;
		double limit;
		double step;
		double page;
		int digits;
		void (*on_update)(Gtk::HScale(&)[4], MRFRobot &, bool);
		void (*get_low_sensitivity_scale_factors)(double (&)[4]);
	} MODES[] = {
		{ u8"Coast", 0x0, 1, 0.1, 0.5, 0, &on_update_coast, &get_low_sensitivity_scale_factors_coast },
		{ u8"Brake", 0x0, 1, 0.1, 0.5, 0, &on_update_brake, &get_low_sensitivity_scale_factors_brake },
		{ u8"Per-motor", 0xF, 1023, 1, 25, 0, &on_update_permotor, &get_low_sensitivity_scale_factors_permotor },
		{ u8"Matrix", 0x7, 20, 0.1, 3, 1, &on_update_matrix, &get_low_sensitivity_scale_factors_matrix },
	};
}

DrivePanel::DrivePanel(MRFDongle &dongle, MRFRobot &robot) : dongle(dongle), robot(robot), controllers_checkbox(u8"Controllers"), force_on_button(u8"Force power on") {
	for (unsigned int i = 0; i < G_N_ELEMENTS(MODES); ++i) {
		mode_chooser.append_text(MODES[i].name);
	}
	coast();
	mode_chooser.signal_changed().connect(sigc::mem_fun(this, &DrivePanel::on_mode_changed));
	pack_start(mode_chooser, Gtk::PACK_SHRINK);
	for (unsigned int i = 0; i < G_N_ELEMENTS(controls); ++i) {
		controls[i].get_adjustment()->signal_value_changed().connect(sigc::mem_fun(this, &DrivePanel::on_update));
		pack_start(controls[i], Gtk::PACK_SHRINK);
	}
	controllers_checkbox.set_active();
	controllers_checkbox.signal_toggled().connect(sigc::mem_fun(this, &DrivePanel::on_update));
	pack_start(controllers_checkbox, Gtk::PACK_SHRINK);
	force_on_button.signal_clicked().connect(sigc::mem_fun(this, &DrivePanel::force_on));
	pack_start(force_on_button, Gtk::PACK_SHRINK);
	on_mode_changed();
}

void DrivePanel::zero() {
	for (unsigned int i = 0; i < G_N_ELEMENTS(controls); ++i) {
		controls[i].get_adjustment()->set_value(0);
	}
}

void DrivePanel::coast() {
	mode_chooser.set_active(0);
}

void DrivePanel::set_values(const double(&values)[4]) {
	for (unsigned int i = 0; i < G_N_ELEMENTS(controls); ++i) {
		if (controls[i].get_sensitive()) {
			controls[i].get_adjustment()->set_value(values[i] * controls[i].get_adjustment()->get_upper());
		}
	}
}

void DrivePanel::get_low_sensitivity_scale_factors(double (&scale)[4]) {
	int row = mode_chooser.get_active_row_number();
	if (row >= 0) {
		MODES[row].get_low_sensitivity_scale_factors(scale);
	}
}

void DrivePanel::on_mode_changed() {
	int row = mode_chooser.get_active_row_number();
	if (row >= 0) {
		for (unsigned int i = 0; i < G_N_ELEMENTS(controls); ++i) {
			controls[i].set_sensitive(!!(MODES[row].sensitive_mask & (1 << i)));
			controls[i].get_adjustment()->configure(0, -MODES[row].limit, MODES[row].limit, MODES[row].step, MODES[row].page, 0);
			controls[i].set_digits(MODES[row].digits);
		}
		on_update();
	}
}

void DrivePanel::on_update() {
	int row = mode_chooser.get_active_row_number();
	if (row >= 0) {
		MODES[row].on_update(controls, robot, controllers_checkbox.get_active());
	}
}

void DrivePanel::force_on() {
	force_on_button.set_label(u8"Sending…");
	force_on_button.set_sensitive(false);
	uint8_t data[1] = { 0x09 };
	force_on_message.reset(new MRFDongle::SendReliableMessageOperation(dongle, robot.index, data, sizeof(data)));
	force_on_message->signal_done.connect(sigc::mem_fun(this, &DrivePanel::check_force_on_result));
}

void DrivePanel::check_force_on_result(AsyncOperation<void> &op) {
	try {
		op.result();
		force_on_button.set_label(u8"OK");
	} catch (const MRFDongle::SendReliableMessageOperation::NotAssociatedError &) {
		force_on_button.set_label(u8"Not associated");
	} catch (const MRFDongle::SendReliableMessageOperation::NotAcknowledgedError &) {
		force_on_button.set_label(u8"Not acknowledged");
	} catch (const MRFDongle::SendReliableMessageOperation::ClearChannelError &) {
		force_on_button.set_label(u8"CCA fail");
	}
	force_on_message.reset();
	force_on_button.set_sensitive();
}

