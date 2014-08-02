#include "test/common/drive.h"
#include "util/algorithm.h"
#include <sigc++/bind_return.h>
#include <sigc++/functors/mem_fun.h>

namespace {
	void on_update_coast(Gtk::HScale(&)[4], Drive::Robot &robot, bool) {
		robot.drive_coast();
	}

	void get_low_sensitivity_scale_factors_coast(double (&scale)[4]) {
		scale[0] = scale[1] = scale[2] = scale[3] = 0;
	}

	void on_update_brake(Gtk::HScale(&)[4], Drive::Robot &robot, bool) {
		robot.drive_brake();
	}

	void get_low_sensitivity_scale_factors_brake(double (&scale)[4]) {
		scale[0] = scale[1] = scale[2] = scale[3] = 0;
	}

	void on_update_permotor(Gtk::HScale(&controls)[4], Drive::Robot &robot, bool controlled) {
		int wheels[G_N_ELEMENTS(controls)];
		for (unsigned int i = 0; i < G_N_ELEMENTS(controls); ++i) {
			wheels[i] = clamp_symmetric(static_cast<int>(controls[i].get_value()), 1023);
		}
		robot.drive(wheels, controlled);
	}

	void get_low_sensitivity_scale_factors_permotor(double (&scale)[4]) {
		scale[0] = scale[1] = scale[2] = scale[3] = 0.1;
	}

	void on_update_matrix(Gtk::HScale(&controls)[4], Drive::Robot &robot, bool controlled) {
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
		scale[0] = 0.05;
		scale[1] = 0.05;
		scale[2] = 0.15;
		scale[3] = 0;
	}

	void on_update_zaxis(Gtk::HScale(&controls)[4], Drive::Robot &robot, bool controlled) {
		static const double vector[4] = { -0.4558, 0.5406, -0.5406, 0.4558 };
		double scalar = controls[0].get_value();
		double output[G_N_ELEMENTS(vector)];
		for (unsigned int i = 0; i < G_N_ELEMENTS(output); ++i) {
			output[i] = scalar * vector[i];
		}
		int w[G_N_ELEMENTS(output)];
		for (unsigned int i = 0; i < G_N_ELEMENTS(w); ++i) {
			w[i] = clamp_symmetric(static_cast<int>(output[i]), 1023);
		}
		robot.drive(w, controlled);
	}

	void get_low_sensitivity_scale_factors_zaxis(double (&scale)[4]) {
		scale[0] = scale[1] = scale[2] = scale[3] = 0.1;
	}

	struct Mode {
		const char *name;
		unsigned int sensitive_mask;
		double limit;
		double step;
		double page;
		int digits;
		void (*on_update)(Gtk::HScale(&)[4], Drive::Robot &, bool);
		void (*get_low_sensitivity_scale_factors)(double (&)[4]);
	};

	const Mode MODES_NO_COAST[] = {
		{ u8"Brake", 0x0, 1, 0.1, 0.5, 0, &on_update_brake, &get_low_sensitivity_scale_factors_brake },
		{ u8"Per-motor", 0xF, 1023, 1, 25, 0, &on_update_permotor, &get_low_sensitivity_scale_factors_permotor },
		{ u8"Matrix", 0x7, 20, 0.1, 1, 1, &on_update_matrix, &get_low_sensitivity_scale_factors_matrix },
		{ u8"Z axis", 0x1, 2047, 1, 25, 0, &on_update_zaxis, &get_low_sensitivity_scale_factors_zaxis },
	};

	const Mode MODES_COAST[] = {
		{ u8"Coast", 0x0, 255, 1, 25, 0, &on_update_coast, &get_low_sensitivity_scale_factors_coast },
		{ u8"Brake", 0x0, 1, 0.1, 0.5, 0, &on_update_brake, &get_low_sensitivity_scale_factors_brake },
		{ u8"Per-motor", 0xF, 1023, 1, 25, 0, &on_update_permotor, &get_low_sensitivity_scale_factors_permotor },
		{ u8"Matrix", 0x7, 20, 0.1, 1, 1, &on_update_matrix, &get_low_sensitivity_scale_factors_matrix },
		{ u8"Z axis", 0x1, 2047, 1, 25, 0, &on_update_zaxis, &get_low_sensitivity_scale_factors_zaxis },
	};
}

DrivePanel::DrivePanel(Drive::Robot &robot) :
		robot(robot),
		controllers_checkbox(u8"Controllers") {
	const Mode * const MODES = robot.can_coast() ? MODES_COAST : MODES_NO_COAST;
	const std::size_t MODES_SIZE = robot.can_coast() ? G_N_ELEMENTS(MODES_COAST) : G_N_ELEMENTS(MODES_NO_COAST);
	for (std::size_t i = 0; i < MODES_SIZE; ++i) {
		mode_chooser.append_text(MODES[i].name);
	}
	coast();
	mode_chooser.signal_changed().connect(sigc::mem_fun(this, &DrivePanel::on_mode_changed));
	pack_start(mode_chooser, Gtk::PACK_SHRINK);
	for (Gtk::HScale &control : controls) {
		control.get_adjustment()->signal_value_changed().connect(sigc::mem_fun(this, &DrivePanel::on_update));
		pack_start(control, Gtk::PACK_SHRINK);
	}
	controllers_checkbox.set_active();
	controllers_checkbox.signal_toggled().connect(sigc::mem_fun(this, &DrivePanel::on_update));
	pack_start(controllers_checkbox, Gtk::PACK_SHRINK);
	on_mode_changed();
	Glib::signal_timeout().connect(sigc::bind_return(sigc::mem_fun(this, &DrivePanel::on_update), true), 1000 / 30);
}

void DrivePanel::zero() {
	for (Gtk::HScale &control : controls) {
		control.get_adjustment()->set_value(0);
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
	const Mode * const MODES = robot.can_coast() ? MODES_COAST : MODES_NO_COAST;
	int row = mode_chooser.get_active_row_number();
	if (row >= 0) {
		MODES[row].get_low_sensitivity_scale_factors(scale);
	}
}

void DrivePanel::on_mode_changed() {
	const Mode * const MODES = robot.can_coast() ? MODES_COAST : MODES_NO_COAST;
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
	const Mode * const MODES = robot.can_coast() ? MODES_COAST : MODES_NO_COAST;
	int row = mode_chooser.get_active_row_number();
	if (row >= 0) {
		MODES[row].on_update(controls, robot, controllers_checkbox.get_active());
	}
}

