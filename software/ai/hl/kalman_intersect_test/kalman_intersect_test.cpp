#include "ai/hl/hl.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include <iostream>
#include <gtkmm.h>

using namespace AI::HL;
using namespace AI::HL::W;

namespace {
	const int NUMBER_OF_TARGETS = 2;
	Point TARGETS[NUMBER_OF_TARGETS] = { Point(0.0,-2.0), Point(0.0,2.0) }; 
	double TARGETS_ORIENT[NUMBER_OF_TARGETS] = {0.5*M_PI,-0.5*M_PI};
	class KalmanIntersectTestFactory : public HighLevelFactory {
		public:
			KalmanIntersectTestFactory() : HighLevelFactory("Hawdy, Kalman Intersect Test") {
			}

			HighLevel::Ptr create_high_level(World &world) const;
	};

	KalmanIntersectTestFactory factory_instance;

	class KalmanIntersectTest : public HighLevel {
		public:
			KalmanIntersectTest(World &world) : world(world), isect_robot_dst(0,0), which_target(0),
				to_dribble(false), to_kick(false){
				dribble_btn.set_label("dribble");
				dribble_btn.signal_clicked().connect( sigc::mem_fun(*this, &KalmanIntersectTest::on_dribble_btn_clicked ) );

				kick_btn.set_label("kick");
				kick_btn.signal_clicked().connect( sigc::mem_fun(*this, &KalmanIntersectTest::on_kick_btn_clicked ) );

				run_btn.set_label("run bot");
				run_btn.signal_clicked().connect( sigc::mem_fun(*this, &KalmanIntersectTest::on_run_btn_clicked ) );

				reset_btn.set_label("Reset");
				reset_btn.signal_clicked().connect( sigc::mem_fun(*this, &KalmanIntersectTest::on_reset_btn_clicked) );
				ui_box.add( reset_btn );
				ui_box.add( run_btn );
				ui_box.add( kick_btn );
				ui_box.add( dribble_btn );
			}

			KalmanIntersectTestFactory &factory() const {
				return factory_instance;
			}

			void tick() {
				// sample ball position once per tick
				Point new_path_point = world.ball().position();
				//path_points.push_back( new_path_point );
				//std::cout << "(" << new_path_point.x << ", " << new_path_point.y <<")\n";
				Point ball_velocity =  world.ball().velocity();
				// this position should take into account of the velocity of the robot and ball PROPERLY, subject to change
				isect_robot_dst = new_path_point + ball_velocity * ball_velocity.len();
				
				// enable the bot to go to the next dst

				if( world.friendly_team().size() == 1 ){
					Player::Ptr player = world.friendly_team().get(0);
					player->move(TARGETS[ which_target ], TARGETS_ORIENT[ which_target ], Point());
					if( to_dribble ){
						player->type(AI::Flags::MoveType::DRIBBLE);
					} else {
						player->type(AI::Flags::MoveType::NORMAL);
					}
					if( to_kick ){
						player->kick(10.0);
						to_kick = false;
					}
				}
			}

			Gtk::Widget *ui_controls() {
				return &ui_box;
			}

			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx){
				ctx->set_source_rgb(1,0.5,0.5);
				if( path_points.size() != 0 ){
					ctx->move_to( path_points.begin()->x, path_points.begin()->y );
				}
				for( std::vector<Point>::iterator iter = path_points.begin(); iter < path_points.end(); ++iter ){
					ctx->line_to( (*iter).x, (*iter).y );
				}
				ctx->set_line_width(0.05);
				ctx->stroke();

				ctx->set_source_rgb(1.0,1.0,0);
				ctx->arc(isect_robot_dst.x, isect_robot_dst.y, 0.3, 0.0, 2*M_PI );
				ctx->stroke();

				return;
			}

		private:
			World &world;
			// may expand this structure to include time stamp
			Gtk::Button reset_btn;
			Gtk::Button run_btn;
			Gtk::Button dribble_btn;
			Gtk::Button kick_btn;
			Gtk::VBox ui_box;
			std::vector<Point> path_points; 
			Point isect_robot_dst;
			
			unsigned int which_target;
			bool to_dribble;
			bool to_kick;
			
			void on_reset_btn_clicked(){
				path_points.clear();
				isect_robot_dst = Point(0,0);
			}

			void on_run_btn_clicked(){
				which_target++;
				which_target = which_target % NUMBER_OF_TARGETS;
			}

			void on_dribble_btn_clicked(){
				to_dribble = !to_dribble;
			}

			void on_kick_btn_clicked(){
				to_kick = true;
			}
	};

	HighLevel::Ptr KalmanIntersectTestFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new KalmanIntersectTest(world));
		return p;
	}
}

