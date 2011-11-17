#include "test/drive.h"
#include "util/algorithm.h"

namespace {
	void on_update_scram(Gtk::HScale(&)[4], XBeeRobot::Ptr robot, bool) {
		robot->drive_scram();
	}

	void get_low_sensitivity_scale_factors_scram(double (&scale)[4]) {
		scale[0] = scale[1] = scale[2] = scale[3] = 0;
	}

	void on_update_permotor(Gtk::HScale(&controls)[4], XBeeRobot::Ptr robot, bool controlled) {
		int wheels[G_N_ELEMENTS(controls)];
		for (unsigned int i = 0; i < G_N_ELEMENTS(controls); ++i) {
			wheels[i] = clamp(static_cast<int>(controls[i].get_value()), -1023, 1023);
		}
		robot->drive(wheels, controlled);
	}

	void get_low_sensitivity_scale_factors_permotor(double (&scale)[4]) {
		scale[0] = scale[1] = scale[2] = scale[3] = 0.1;
	}

	void on_update_matrix(Gtk::HScale(&controls)[4], XBeeRobot::Ptr robot, bool controlled) {
		static const double matrix[4][3] = {
			{ -42.5995, 27.6645, 4.3175 },
			{ -35.9169, -35.9169, 4.3175 },
			{ 35.9169, -35.9169, 4.3175 },
			{ 42.5995, 27.6645, 4.3175 }
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
			w[i] = clamp(static_cast<int>(output[i]), -1023, 1023);
		}
		robot->drive(w, controlled);
	}

	void get_low_sensitivity_scale_factors_matrix(double (&scale)[4]) {
		scale[0] = 0.2;
		scale[1] = 0.2;
		scale[2] = 0.5;
		scale[3] = 0;
	}

	const struct {
		const char *name;
		unsigned int sensitive_mask;
		double limit;
		double step;
		double page;
		int digits;
		void (*on_update)(Gtk::HScale(&)[4], XBeeRobot::Ptr, bool);
		void (*get_low_sensitivity_scale_factors)(double (&)[4]);
	} MODES[] = {
		{ "Scram", 0x0, 1, 0.1, 0.5, 0, &on_update_scram, &get_low_sensitivity_scale_factors_scram },
		{ "Per-motor", 0xF, 1023, 1, 25, 0, &on_update_permotor, &get_low_sensitivity_scale_factors_permotor },
		{ "Matrix", 0x7, 20, 0.1, 3, 1, &on_update_matrix, &get_low_sensitivity_scale_factors_matrix },
	};
}

DrivePanel::DrivePanel(XBeeRobot::Ptr robot) : robot(robot), controllers_checkbox("Controllers") {
	for (unsigned int i = 0; i < G_N_ELEMENTS(MODES); ++i) {
		mode_chooser.append_text(MODES[i].name);
	}
	scram();
	mode_chooser.signal_changed().connect(sigc::mem_fun(this, &DrivePanel::on_mode_changed));
	pack_start(mode_chooser, Gtk::PACK_SHRINK);
	for (unsigned int i = 0; i < G_N_ELEMENTS(controls); ++i) {
		controls[i].get_adjustment()->signal_value_changed().connect(sigc::mem_fun(this, &DrivePanel::on_update));
		pack_start(controls[i], Gtk::PACK_SHRINK);
	}
	controllers_checkbox.set_active();
	controllers_checkbox.signal_toggled().connect(sigc::mem_fun(this, &DrivePanel::on_update));
	pack_start(controllers_checkbox, Gtk::PACK_SHRINK);
	on_mode_changed();
}

void DrivePanel::zero() {
	for (unsigned int i = 0; i < G_N_ELEMENTS(controls); ++i) {
		controls[i].get_adjustment()->set_value(0);
	}
}

void DrivePanel::scram() {
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

