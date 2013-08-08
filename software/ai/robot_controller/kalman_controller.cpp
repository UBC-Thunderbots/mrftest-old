/* ****************************

   Mis-usage of this class would harm the robots that and could ultimately cause other personnels on the team to hurt you.

   Please talk to koko before running this class

 ******************************** */

#include "ai/robot_controller/robot_controller.h"
#include "geom/angle.h"
#include "geom/point.h"
#include "util/noncopyable.h"
#include "util/dprint.h"
#include "util/param.h"
#include <cmath>
#include <glibmm/ustring.h>
#include <gtkmm/adjustment.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/checkbutton.h>
#include <gtkmm/label.h>
#include <gtkmm/scale.h>
#include <gtkmm/togglebutton.h>
#include <gtkmm/window.h>

using AI::RC::RobotController;
using AI::RC::RobotControllerFactory;
using namespace AI::RC::W;

namespace {
	class KalmanController : public RobotController {
		public:
			explicit KalmanController(World world, Player player) : RobotController(world, player), lbl_ramp_time(u8"T ramp"), lbl_plateau_time(u8"T plateau"), lbl_terminal_velocity(u8"V terminal"), lbl_direction(u8"Direction"), lbl_rotate_speed(u8"Rotation"), lbl_pivot_radius(u8"Pivot Radius"), adj_ramp_time(0.0, 0.0, 2.0, 0.1, 0.5, 1.0), adj_plateau_time(0.0, 0.0, 4.0, 0.2, 1.0, 1.0), adj_terminal_velocity(0.0, 0.0, 10.0, 0.1, 0.2, 0.2), adj_direction(0.0, 0.0, 2 * M_PI, 0.1 * M_PI, 0.5 * M_PI, 1.0), adj_rotate_speed(0.0, -20 * M_PI, 20 * M_PI, 0.05 * M_PI, 0.1 * M_PI, 0.0), adj_pivot_radius(0.1, 0.1, 10.0, 0.1, 1.0, 0.0), hsb_ramp_time(adj_ramp_time), hsb_plateau_time(adj_plateau_time), hsb_terminal_velocity(adj_terminal_velocity), hsb_direction(adj_direction), hsb_rotate_speed(adj_rotate_speed), hsb_pivot_radius(adj_pivot_radius), enable_pivot_radius(true), velocity_inc(0.0, 0.0), to_be_ramp_time(0.0), to_be_plateau_time(0.0), to_be_terminal_velocity(0.0), to_be_velocity(0.0, 0.0), to_be_direction(Angle::zero()), to_be_rotate_speed(Angle::zero()), to_be_pivot_radius(0.1), state(State::IDLE) {
				enable_pivot_radius_tgl.signal_toggled().connect(sigc::mem_fun(*this, &KalmanController::on_enable_pivot_radius_toggled));

				// param bar
				adj_ramp_time.signal_value_changed().connect(sigc::mem_fun(*this, &KalmanController::on_adj_ramp_time_changed));
				adj_plateau_time.signal_value_changed().connect(sigc::mem_fun(*this, &KalmanController::on_adj_plateau_time_changed));
				adj_terminal_velocity.signal_value_changed().connect(sigc::mem_fun(*this, &KalmanController::on_adj_terminal_velocity_changed));
				adj_direction.signal_value_changed().connect(sigc::mem_fun(*this, &KalmanController::on_adj_direction_changed));
				adj_rotate_speed.signal_value_changed().connect(sigc::mem_fun(*this, &KalmanController::on_adj_rotate_speed_changed));
				adj_pivot_radius.signal_value_changed().connect(sigc::mem_fun(*this, &KalmanController::on_adj_pivot_radius_changed));
				// hsb_ramp_time.set_label(u8"T ramp");
				// hsb_plateau_time.set_label(u8"T plateau");
				// hsb_terminal_velocity.set_label(u8"v terminal");
				// hsb_direction.set_label(u8"direction");
				ui_box.add(lbl_terminal_velocity);
				ui_box.add(hsb_terminal_velocity);
				ui_box.add(lbl_direction);
				ui_box.add(hsb_direction);
				ui_box.add(lbl_ramp_time);
				ui_box.add(hsb_ramp_time);
				ui_box.add(lbl_plateau_time);
				ui_box.add(hsb_plateau_time);
				ui_box.add(lbl_rotate_speed);
				ui_box.add(hsb_rotate_speed);
				ui_box.add(lbl_pivot_radius);
				ui_box.add(hsb_pivot_radius);


				enable_pivot_radius_tgl.set_label(u8"Enable pivot");
				ui_box.add(enable_pivot_radius_tgl);

				// setup test drive button
				test_drive_btn.set_label(u8"Drive me.");
				test_drive_btn.signal_clicked().connect(sigc::mem_fun(*this, &KalmanController::on_test_drive_btn_clicked));
				ui_box.add(test_drive_btn);

				pop.add(ui_box);
				pop.show_all();
			}

			void tick() {
				int wheel_speeds[4] = { 0, 0, 0, 0 };

				if (state != State::IDLE) {
					convert_to_wheels(to_be_velocity, to_be_rotate_speed, wheel_speeds);
					LOG_INFO(Glib::ustring::compose(u8"%1\t%2\t%3 %4 (%5, %6, %7, %8)", to_be_pivot_radius, to_be_terminal_velocity, to_be_velocity, to_be_rotate_speed, wheel_speeds[0], wheel_speeds[1], wheel_speeds[2], wheel_speeds[3]));
				}
				player.drive(wheel_speeds);


				pop.show();
			}

			enum class State {
				IDLE,
				RUN,
				PIVOT
			};

		private:
			Gtk::Window pop;
			Gtk::VBox ui_box;

			Gtk::Button reset_btn;
			Gtk::CheckButton enable_pivot_radius_tgl;

			Gtk::ToggleButton set_param_tgl;
			Gtk::Label lbl_ramp_time;
			Gtk::Label lbl_plateau_time;
			Gtk::Label lbl_terminal_velocity;
			Gtk::Label lbl_direction;
			Gtk::Label lbl_rotate_speed;
			Gtk::Label lbl_pivot_radius;

			Gtk::Adjustment adj_ramp_time;
			Gtk::Adjustment adj_plateau_time;
			Gtk::Adjustment adj_terminal_velocity;
			Gtk::Adjustment adj_direction;
			Gtk::Adjustment adj_rotate_speed;
			Gtk::Adjustment adj_pivot_radius;

			Gtk::HScale hsb_ramp_time;
			Gtk::HScale hsb_plateau_time;
			Gtk::HScale hsb_terminal_velocity;
			Gtk::HScale hsb_direction;
			Gtk::HScale hsb_rotate_speed;
			Gtk::HScale hsb_pivot_radius;

			Gtk::Button test_drive_btn;

			bool enable_pivot_radius;
			Point current_tick_velocity;
			Point velocity_inc;

			double to_be_ramp_time;
			double to_be_plateau_time;
			double to_be_terminal_velocity;
			Point to_be_velocity;
			Angle to_be_direction;
			Angle to_be_rotate_speed;
			double to_be_pivot_radius;

			State state;

			void on_enable_pivot_radius_toggled() {
				if (enable_pivot_radius_tgl.get_active()) {
					enable_pivot_radius = true;
					to_be_rotate_speed = Angle::of_radians(to_be_terminal_velocity / to_be_pivot_radius);
				} else {
					enable_pivot_radius = false;
				}
			}

			void on_test_drive_btn_clicked() {
				if (state == State::IDLE) {
					if (enable_pivot_radius) {
						state = State::PIVOT;
						to_be_velocity = Point::of_angle(player.orientation()) * to_be_terminal_velocity;
						to_be_rotate_speed = Angle::of_radians(to_be_terminal_velocity / to_be_pivot_radius);
						test_drive_btn.set_label(u8"Stop Pivot");
					} else {
						state = State::RUN;
						test_drive_btn.set_label(u8"Stop");
					}
				} else {
					state = State::IDLE;
					test_drive_btn.set_label(u8"Drive");
				}
			}

			void on_adj_ramp_time_changed() {
				to_be_ramp_time = hsb_ramp_time.get_value();
			}
			void on_adj_plateau_time_changed() {
				to_be_plateau_time = hsb_plateau_time.get_value();
			}
			void on_adj_terminal_velocity_changed() {
				to_be_terminal_velocity = hsb_terminal_velocity.get_value();
				if (!enable_pivot_radius) {
					to_be_velocity = Point::of_angle(to_be_direction) * to_be_terminal_velocity;
					LOG_INFO(Glib::ustring::format(to_be_velocity));
				} else {
					to_be_velocity = Point::of_angle(player.orientation()) * to_be_terminal_velocity;
					to_be_rotate_speed = Angle::of_radians(to_be_terminal_velocity / to_be_pivot_radius);
				}
			}
			void on_adj_direction_changed() {
				to_be_direction = Angle::of_radians(hsb_direction.get_value());
				if (!enable_pivot_radius) {
					to_be_velocity = Point::of_angle(to_be_direction) * to_be_terminal_velocity;
					LOG_INFO(Glib::ustring::format(to_be_velocity));
				}
			}
			void on_adj_rotate_speed_changed() {
				to_be_rotate_speed = Angle::of_radians(hsb_rotate_speed.get_value());
			}
			void on_adj_pivot_radius_changed() {
				to_be_pivot_radius = hsb_pivot_radius.get_value();
				if (enable_pivot_radius) {
					to_be_rotate_speed = Angle::of_radians(to_be_terminal_velocity / to_be_pivot_radius);
				}
			}
	};

	typedef KalmanController::State State;
}

ROBOT_CONTROLLER_REGISTER(KalmanController)
