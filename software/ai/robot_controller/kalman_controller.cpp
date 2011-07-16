/* ****************************

   Mis-usage of this class would harm the robots that and could ultimately cause other personnels on the team to hurt you.

   Please talk to koko before running this class

 ******************************** */

#include "ai/robot_controller/robot_controller.h"
#include "geom/angle.h"
#include "geom/point.h"
#include "util/byref.h"
#include "util/noncopyable.h"
#include "util/dprint.h"
#include "util/param.h"
#include <iostream>
#include <cmath>
#include <glibmm.h>
#include <gtkmm.h>
#include <map>

using AI::RC::RobotController;
using AI::RC::RobotControllerFactory;
using namespace AI::RC::W;

namespace {
	double WHEEL_ORIENT[4] = { -0.25 * M_PI, -0.75 * M_PI, -1.25 * M_PI, -1.75 * M_PI };

	class KalmanController : public RobotController {
		public:
			KalmanController(World &world, Player::Ptr player) : RobotController(world, player), velocity_inc(0.0, 0.0), state(State::Idle), adj_ramp_time(0.0, 0.0, 2.0, 0.1, 0.5, 1.0), hsb_ramp_time(adj_ramp_time), lbl_ramp_time("T ramp"), to_be_ramp_time(0.0), adj_plateau_time(0.0, 0.0, 4.0, 0.2, 1.0, 1.0), hsb_plateau_time(adj_plateau_time), lbl_plateau_time("T plateau"), to_be_plateau_time(0.0), adj_terminal_velocity(0.0, 0.0, 10.0, 0.1, 0.2, 0.2), hsb_terminal_velocity(adj_terminal_velocity), lbl_terminal_velocity("V terminal"), to_be_terminal_velocity(0.0), adj_direction(0.0, 0.0, 2 * M_PI, 0.1 * M_PI, 0.5 * M_PI, 1.0), hsb_direction(adj_direction), lbl_direction("Direction"), to_be_direction(0.0), adj_rotate_speed(0.0, -20 * M_PI, 20 * M_PI, 0.05 * M_PI, 0.1 * M_PI, 0.0), hsb_rotate_speed(adj_rotate_speed), lbl_rotate_speed("Rotation"), to_be_rotate_speed(0.0), adj_pivot_radius(0.1, 0.1, 10.0, 0.1, 1.0, 0.0), hsb_pivot_radius(adj_pivot_radius), lbl_pivot_radius("Pivot Radius"), to_be_pivot_radius(0.1), to_be_velocity(0.0, 0.0), enable_pivot_radius(true) {
				enable_pivot_radius_tgl.signal_toggled().connect(sigc::mem_fun(*this, &KalmanController::on_enable_pivot_radius_toggled));

				// param bar
				adj_ramp_time.signal_value_changed().connect(sigc::mem_fun(*this, &KalmanController::on_adj_ramp_time_changed));
				adj_plateau_time.signal_value_changed().connect(sigc::mem_fun(*this, &KalmanController::on_adj_plateau_time_changed));
				adj_terminal_velocity.signal_value_changed().connect(sigc::mem_fun(*this, &KalmanController::on_adj_terminal_velocity_changed));
				adj_direction.signal_value_changed().connect(sigc::mem_fun(*this, &KalmanController::on_adj_direction_changed));
				adj_rotate_speed.signal_value_changed().connect(sigc::mem_fun(*this, &KalmanController::on_adj_rotate_speed_changed));
				adj_pivot_radius.signal_value_changed().connect(sigc::mem_fun(*this, &KalmanController::on_adj_pivot_radius_changed));
				// hsb_ramp_time.set_label("T ramp");
				// hsb_plateau_time.set_label("T plateau");
				// hsb_terminal_velocity.set_label("v terminal");
				// hsb_direction.set_label("direction");
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


				enable_pivot_radius_tgl.set_label("Enable pivot");
				ui_box.add(enable_pivot_radius_tgl);

				// setup test drive button
				test_drive_btn.set_label("Drive me.");
				test_drive_btn.signal_clicked().connect(sigc::mem_fun(*this, &KalmanController::on_test_drive_btn_clicked));
				ui_box.add(test_drive_btn);

				pop.add(ui_box);
				pop.show_all();
			}

			void tick() {
				int wheel_speeds[4] = { 0, 0, 0, 0 };

				if (state != State::Idle) {
					convert_to_wheels(to_be_velocity, to_be_rotate_speed, wheel_speeds);
					std::cout << to_be_pivot_radius << "\t" << to_be_terminal_velocity << "\t(" << to_be_velocity.x << ", " << to_be_velocity.y << ") " << to_be_rotate_speed << " (" << wheel_speeds[0] << ", " << wheel_speeds[1] << ", " << wheel_speeds[2] << ", " << wheel_speeds[3] << ")" << std::endl;
				}
				player->drive(wheel_speeds);


				pop.show();
			}

			enum State {
				Idle,
				Run,
				Pivot
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
			unsigned int frame_count;
			unsigned int ramp_frame;
			unsigned int plateau_frame;
			double terminal_speed;
			Point current_tick_velocity;
			double current_tick_rotate_velocity;
			Point velocity_inc;

			double to_be_ramp_time;
			double to_be_plateau_time;
			double to_be_terminal_velocity;
			Point to_be_velocity;
			double to_be_direction;
			double to_be_rotate_speed;
			double to_be_pivot_radius;

			State state;

			void on_enable_pivot_radius_toggled() {
				if (enable_pivot_radius_tgl.get_active()) {
					enable_pivot_radius = true;
					to_be_rotate_speed = to_be_terminal_velocity / to_be_pivot_radius;
				} else {
					enable_pivot_radius = false;
				}
			}

			void on_test_drive_btn_clicked() {
				if (state == State::Idle) {
					if (enable_pivot_radius) {
						state = State::Pivot;
						to_be_velocity = Point::of_angle(player->orientation()) * to_be_terminal_velocity;
						to_be_rotate_speed = to_be_terminal_velocity / to_be_pivot_radius;
						test_drive_btn.set_label("Stop Pivot");
					} else {
						state = State::Run;
						test_drive_btn.set_label("Stop");
					}
				} else {
					state = State::Idle;
					test_drive_btn.set_label("Drive");
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
					std::cout << to_be_velocity.x << " " << to_be_velocity.y << std::endl;
				} else {
					to_be_velocity = Point::of_angle(player->orientation()) * to_be_terminal_velocity;
					to_be_rotate_speed = to_be_terminal_velocity / to_be_pivot_radius;
				}
			}
			void on_adj_direction_changed() {
				to_be_direction = hsb_direction.get_value();
				if (!enable_pivot_radius) {
					to_be_velocity = Point::of_angle(to_be_direction) * to_be_terminal_velocity;
					std::cout << to_be_velocity.x << " " << to_be_velocity.y << std::endl;
				}
			}
			void on_adj_rotate_speed_changed() {
				to_be_rotate_speed = hsb_rotate_speed.get_value();
			}
			void on_adj_pivot_radius_changed() {
				to_be_pivot_radius = hsb_pivot_radius.get_value();
				if (enable_pivot_radius) {
					to_be_rotate_speed = to_be_terminal_velocity / to_be_pivot_radius;
				}
			}
	};

	typedef KalmanController::State State;


	class KalmanControllerFactory : public RobotControllerFactory {
		public:
			KalmanControllerFactory() : RobotControllerFactory("Kalman Test") {
			}

			RobotController::Ptr create_controller(World &world, Player::Ptr player) const {
				RobotController::Ptr p(new KalmanController(world, player));
				return p;
			}
	};

	KalmanControllerFactory factory;
}

