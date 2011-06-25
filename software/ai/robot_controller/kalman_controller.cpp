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
	double WHEEL_ORIENT[4] = { -0.25*M_PI, -0.75* M_PI, -1.25*M_PI, -1.75*M_PI };

	class KalmanController : public RobotController {
		public:
			KalmanController(World &world, Player::Ptr player) : RobotController(world, player), 
						velocity_inc(0.0,0.0), state(State::Idle),
						adj_ramp_time(0.0, 0.0, 2.0, 0.1, 0.5, 1.0), hsb_ramp_time(adj_ramp_time), lbl_ramp_time("T ramp"), to_be_ramp_time(0.0),
						adj_plateau_time(0.0, 0.0, 4.0, 0.2, 1.0, 1.0), hsb_plateau_time(adj_plateau_time), lbl_plateau_time("T plateau"), to_be_plateau_time(0.0),
						adj_terminal_velocity(0.0, 0.0, 1.0, 0.1, 0.2, 0.2), hsb_terminal_velocity(adj_terminal_velocity), lbl_terminal_velocity("V terminal"), to_be_terminal_velocity(0.0),
						adj_direction(0.0, 0.0, 2*M_PI, 0.1*M_PI, 0.5*M_PI, 1.0), hsb_direction(adj_direction), lbl_direction("Direction"), to_be_direction(0.0),
						adj_rotate_speed(0.0, -20*M_PI, 20*M_PI, 0.05*M_PI, 0.1*M_PI, 0.0), hsb_rotate_speed(adj_rotate_speed), lbl_rotate_speed("Rotation"), to_be_rotate_speed(0.0) {

				// disable parameter setting
				set_param_tgl.set_label("Test drive param");
				set_param_tgl.set_active(false);
				set_param_tgl.signal_toggled().connect( sigc::mem_fun(*this, &KalmanController::on_set_param_toggled) );
				ui_box.add( set_param_tgl );

				// param bar
				adj_rotate_speed.signal_value_changed().connect( sigc::mem_fun(*this, &KalmanController::on_adj_rotate_speed_changed) );

				hsb_ramp_time.set_can_focus(false);
				hsb_plateau_time.set_can_focus(false);
				hsb_terminal_velocity.set_can_focus(false);
				hsb_direction.set_can_focus(false);
				//hsb_ramp_time.set_label("T ramp");
				//hsb_plateau_time.set_label("T plateau");
				//hsb_terminal_velocity.set_label("v terminal");
				//hsb_direction.set_label("direction");
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

				// setup test drive button
				test_drive_btn.set_label("Drive me.");
				test_drive_btn.signal_clicked().connect( sigc::mem_fun(*this, &KalmanController::on_test_drive_btn_clicked) );
				ui_box.add( test_drive_btn );
				test_drive_rotate_btn.set_label("Drive");
				test_drive_rotate_btn.signal_clicked().connect( sigc::mem_fun(*this, &KalmanController::on_test_drive_rotate_btn_clicked) );
				ui_box.add( test_drive_rotate_btn);

				pop.add(ui_box);
				pop.show_all();
			}

			void tick() {

				int wheel_speeds[4];

				if( state != State::Idle ){
					if( state == State::Drive_in_acc ){
						if( frame_count > ramp_frame ){
							current_tick_velocity += velocity_inc;
							state = State::Drive_in_progress;
							frame_count = 0;
						} else {
							current_tick_velocity += velocity_inc;
							frame_count++;
						}
					}
					if( state == State::Drive_in_progress ){
						if( frame_count > plateau_frame ){
							state = State::Drive_in_dacc;
							frame_count = 0;
						} else {
							frame_count++;
						}
					}
					if( state == State::Drive_in_dacc ){
						if( frame_count > ramp_frame ){
							current_tick_velocity = Point(0,0);
							state = State::Idle;
							frame_count = 0;
							test_drive_btn.set_label("Drive me.");
						} else {
							current_tick_velocity -= velocity_inc;
							frame_count++;
						}
					}
					if( state == State::Rotate ){
						current_tick_rotate_velocity = to_be_rotate_speed;
					}

					std::vector<int> tmp1 = vel2wheels(current_tick_velocity);
					std::vector<int> tmp2 = rot2wheels(current_tick_rotate_velocity);
					wheel_speeds[0] = tmp1[0] + tmp2[0];
					wheel_speeds[1] = tmp1[1] + tmp2[1];
					wheel_speeds[2] = tmp1[2] + tmp2[2];
					wheel_speeds[3] = tmp1[3] + tmp2[3];
					std::cout << "("  << wheel_speeds[0] << ", " << wheel_speeds[1] << ", " << wheel_speeds[2] << ", " << wheel_speeds[3] << ")" << std::endl;

				} else {
					wheel_speeds = { 0, 0, 0, 0 };
				}
				player->drive(wheel_speeds);


				pop.show();

			}
	
			enum State {	
				Idle,
				Setting_drving_param,
				Drive_in_progress,
				Drive_in_acc,
				Drive_in_dacc,
				Rotate
			};

			/*String State_tag[4] = {
				"Idle",
				"Setting driving param".
				"Driving in progress",
				"Driving in acc",
				"Driving in dacc"
			};*/

		private:
			Gtk::Window pop;
			Gtk::VBox ui_box;		

			Gtk::Button reset_btn;			
			Gtk::CheckButton enable_ball_overlay_tgl;

			Gtk::ToggleButton set_param_tgl;
			Gtk::Label lbl_ramp_time;
			Gtk::Label lbl_plateau_time;
			Gtk::Label lbl_terminal_velocity;
			Gtk::Label lbl_direction;
			Gtk::Label lbl_rotate_speed;
			Gtk::Adjustment adj_ramp_time;
			Gtk::Adjustment adj_plateau_time;
			Gtk::Adjustment adj_terminal_velocity;
			Gtk::Adjustment adj_direction;
			Gtk::Adjustment adj_rotate_speed;
			Gtk::HScale hsb_ramp_time;
			Gtk::HScale hsb_plateau_time;
			Gtk::HScale hsb_terminal_velocity;
			Gtk::HScale hsb_direction;
			Gtk::HScale hsb_rotate_speed;
			Gtk::Button test_drive_btn;
			Gtk::Button test_drive_rotate_btn;

			
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
			double to_be_direction;
			double to_be_rotate_speed;

			State state;	

			std::vector<int> vel2wheels( Point vel ){
				//std::cout << vel << std::endl;
				std::vector<int> wheels;
				double bot_orient = player->orientation();
				double vel_orient = vel.orientation();
				double del_orient = vel_orient - bot_orient;
				double wheel_angle_rel[4] = {	WHEEL_ORIENT[0] - del_orient, 
								WHEEL_ORIENT[1] - del_orient,
								WHEEL_ORIENT[2] - del_orient,
								WHEEL_ORIENT[3] - del_orient};
				double vel_len = vel.len();
				for( int i = 0; i<4; i++ ){
					wheels.push_back( 10*std::round(std::sin(wheel_angle_rel[i])*vel_len) );
					//std::cout << wheels[i] << " " ;
				}
				return wheels;
			}

			std::vector<int> rot2wheels( double rot ){
				std::vector<int> wheels;
				int wheel_speed = int( rot/0.2 );// magic constant to be taken away here
				for( int i = 0; i < 4 ; i++ ){
					wheels.push_back(wheel_speed);
				}
				return wheels;
			}

			void on_set_param_toggled(){
				if( !set_param_tgl.get_active() ) {

					frame_count = 0;
					ramp_frame = hsb_ramp_time.get_value() *60;
					plateau_frame = hsb_plateau_time.get_value() * 60;
					terminal_speed = hsb_terminal_velocity.get_value();
					velocity_inc = Point::of_angle(hsb_direction.get_value()) * terminal_speed;

					hsb_ramp_time.set_can_focus(false);
					hsb_plateau_time.set_can_focus(false);
					hsb_terminal_velocity.set_can_focus(false);
					hsb_direction.set_can_focus(false);

					set_param_tgl.set_label("Param Set");
				} else {
					set_param_tgl.set_label("Param In Progress");

					hsb_ramp_time.set_can_focus(true);
					hsb_plateau_time.set_can_focus(true);
					hsb_terminal_velocity.set_can_focus(true);
					hsb_direction.set_can_focus(true);
				}
			}

			void on_test_drive_btn_clicked(){
				if( state == State::Idle ){
					state = State::Drive_in_acc;
					test_drive_btn.set_label("Stop Drive!");					
				} else if ( state == State::Drive_in_progress || state == State::Drive_in_acc ){
					state = State::Drive_in_dacc;
					test_drive_btn.set_label("Stopping.");
				}
				
			}
			
			void on_test_drive_rotate_btn_clicked(){
				if( state == State::Idle ){
					state = State::Rotate;
					test_drive_rotate_btn.set_label("Stop");
				} else if( state == Rotate ){
					state = State::Idle;
					test_drive_rotate_btn.set_label("Drive");
				}
			}

			void on_adj_ramp_time_changed(){
				to_be_ramp_time = hsb_ramp_time.get_value();
			}
			void on_adj_plateau_time_changed(){
				to_be_plateau_time = hsb_plateau_time.get_value();
			}
			void on_adj_terminal_velocity_changed(){
				to_be_terminal_velocity = hsb_terminal_velocity.get_value();
			}
			void on_adj_direction_changed(){
				to_be_direction = hsb_direction.get_value();
			}
			void on_adj_rotate_speed_changed(){
				to_be_rotate_speed = hsb_rotate_speed.get_value();
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

