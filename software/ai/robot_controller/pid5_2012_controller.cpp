#include "ai/robot_controller/robot_controller.h"
#include "ai/robot_controller/tunable_controller.h"
#include "geom/angle.h"
#include "geom/point.h"
#include "util/algorithm.h"
#include "util/noncopyable.h"
#include "util/param.h"
#include "util/timestep.h"
#include <cmath>
#include <vector>
#include <iomanip>
#include <sstream>
#include <gtkmm/expander.h>
#include <gtkmm/table.h>
#include <gtkmm/label.h>
#include <gtkmm/entry.h>
#include <glibmm/refptr.h>
#include <glibmm/ustring.h>

using AI::RC::RobotController;
using AI::RC::OldRobotController;
using AI::RC::RobotControllerFactory;
using namespace AI::RC::W;

namespace {
	DoubleParam pid_xy_prop("xy +proportional", "RC/PID5_2012", 25.0, 0.0, 100.0);
	DoubleParam pid_xy_diff("xy -differential", "RC/PID5_2012", 0.0, -100.0, 10.0);

	DoubleParam pid_a_prop("angular +proportional", "RC/PID5_2012", 30.0, 0.0, 100);
	DoubleParam pid_a_diff("angular -differential", "RC/PID5_2012", 0.2, -100, 100);

	DoubleParam pid_xy_ratio("x to y ratio", "RC/PID5_2012", 0.81, 0.0, 2.0);

	DoubleParam pid_ya_ratio("YA ratio", "RC/PID5_2012", 0.08, -10.0, 10.0);

	DoubleParam wheel_max_speed("Limit wheel speed", "RC/PID5_2012", 100.0, 0, 8888);
	DoubleParam wheel_max_accel("Limit wheel accel", "RC/PID5_2012", 5.0, 0, 8888);
	
	class PID5_2012ControllerFactory;

	class PID5_2012Controller : public RobotController {
		public:
			explicit PID5_2012Controller(World &world, Player::Ptr plr, Gtk::Entry &std_entry);

			~PID5_2012Controller();

			void tick();
			void move(const Point &new_position, Angle new_orientation, Point &linear_velocity, Angle &angular_velocity);
			void move(const Point &new_position, Angle new_orientation, int(&wheel_speeds)[4]);
			void clear();
			void convert(const Point &vel, Angle avel, int(&wheel_speeds)[4]);
			void update_k_monitor();

		private:
			Gtk::Entry &std_entry;

		protected:
			double prev_speed[4];
	};

	PID5_2012Controller::PID5_2012Controller(World &world, Player::Ptr plr, Gtk::Entry &std_entry) : RobotController(world, plr), std_entry(std_entry), prev_speed{0, 0, 0, 0} {
		std_entry.set_sensitive(true);
	}
	
	PID5_2012Controller::~PID5_2012Controller(){
		std_entry.set_sensitive(false);
	}


	void PID5_2012Controller::convert(const Point &vel, Angle avel, int(&wheel_speeds)[4]) {
		static const double WHEEL_MATRIX[4][3] = {
			{ -42.5995, 27.6645, 4.3175 },
			{ -35.9169, -35.9169, 4.3175 },
			{ 35.9169, -35.9169, 4.3175 },
			{ 42.5995, 27.6645, 4.3175 }
		};
		const double input[3] = { vel.x, vel.y, avel.to_radians() };
		double output[4] = { 0, 0, 0, 0 };
		for (unsigned int row = 0; row < 4; ++row) {
			for (unsigned int col = 0; col < 3; ++col) {
				output[row] += WHEEL_MATRIX[row][col] * input[col];
			}
		}

		double max_speed = 0;
		for (unsigned int row = 0; row < 4; ++row) {
			max_speed = std::max(max_speed, std::fabs(output[row]));
		}

		if (max_speed > wheel_max_speed) {
			double ratio = wheel_max_speed / max_speed;
			for (unsigned int row = 0; row < 4; ++row) {
				output[row] *= ratio;
			}
		}

		double accel[4];

		double max_accel = 0;
		for (unsigned int row = 0; row < 4; ++row) {
			accel[row] = output[row] - prev_speed[row];
			max_accel = std::max(max_accel, std::fabs(accel[row]));
		}

		if (max_accel > wheel_max_accel) {
			for (unsigned int i = 0; i < 4; ++i) {
				accel[i] /= max_accel;
				accel[i] *= wheel_max_accel;
				output[i] = prev_speed[i] + accel[i];
			}
		}

		for (unsigned int row = 0; row < 4; ++row) {
			wheel_speeds[row] = static_cast<int>(output[row]);
		}
	}

	void PID5_2012Controller::tick() {
		const AI::RC::W::Player::Path &path = player->path();
		if (path.empty()) {
			clear();
		} else {
			int wheels[4];
			move(path[0].first.first, path[0].first.second, wheels);
			player->drive(wheels);
		}
		
		
		update_k_monitor();
	}

	void PID5_2012Controller::move(const Point &new_position, Angle new_orientation, int(&wheel_speeds)[4]) {
		Point vel;
		Angle avel;
		move(new_position, new_orientation, vel, avel);
		convert(vel, avel, wheel_speeds);
	}

	void PID5_2012Controller::move(const Point &new_position, Angle new_orientation, Point &linear_velocity, Angle &angular_velocity) {
		const Point &current_position = player->position();
		const Angle current_orientation = player->orientation();

		// relative new direction and angle
		Angle new_da = (new_orientation - current_orientation).angle_mod();
		const Point &new_dir = (new_position - current_position).rotate(-current_orientation);

		if (new_da > Angle::HALF) {
			new_da -= Angle::FULL;
		}

		const double px = new_dir.x;
		const double py = new_dir.y;
		const Angle pa = new_da;
		Point vel = (player->velocity()).rotate(-current_orientation);
		double vx = -vel.x;
		double vy = -vel.y;
		Angle va = -player->avelocity();

		linear_velocity.x = px * pid_xy_prop + vx * pid_xy_diff;
		linear_velocity.y = (py * pid_xy_prop + vy * pid_xy_diff) * pid_xy_ratio;

		angular_velocity = pa * pid_a_prop + va * pid_a_diff + Angle::of_radians(linear_velocity.y * pid_ya_ratio);
	}

	void PID5_2012Controller::clear() {
	}

	void PID5_2012Controller::update_k_monitor() {
		Point p_stdev = player->position_stdev();
		Angle o_stdev = player->orientation_stdev();
		Glib::ustring text = Glib::ustring::format( std::scientific, std::setprecision(2), p_stdev, o_stdev);
		std_entry.set_text(text);
	}

	class PID5_2012ControllerFactory : public RobotControllerFactory {
		public:
			explicit PID5_2012ControllerFactory() : RobotControllerFactory("PID 5 2012") {
			}

			std::unique_ptr<RobotController> create_controller(World &world, Player::Ptr plr) const {
				std::unique_ptr<RobotController> p(new PID5_2012Controller(world, plr, get_std(plr->pattern())));
				return p;
			}

			Gtk::Widget *ui_controls() {
				return &get_widget();
			}
	
		private:
			
			Gtk::Widget &get_widget(){
				static Gtk::Expander expander("Kalman Monitor");
				expander.add( get_table() );
				return expander;
			}
	
			Gtk::Table &get_table() const {
				static Gtk::Table t(16, 2);
				static bool initialized = false;
				if (!initialized) {
					for (unsigned int i = 0; i < 16; ++i) {
						t.attach(get_label(i), 0, 1, i, i + 1);
						t.attach(get_std(i), 1, 2, i, i + 1);
					}
					initialized = true;
				}
				return t;
			}

			Gtk::Label &get_label(unsigned int idx) const {
				static Gtk::Label labels[16];
				static bool initialized = false;
				if (!initialized) {
					for (unsigned int i = 0; i < 16; ++i) {
						labels[i].set_text(todecu(i));
					}
					initialized = true;
				}
				return labels[idx];
			}

			Gtk::Entry &get_std(unsigned int idx) const {
				static Gtk::Entry entries[16];
				entries[idx].set_sensitive(false);
				return entries[idx];
			}
	};
}

PID5_2012ControllerFactory PID5_2012ControllerFactory_instance;

