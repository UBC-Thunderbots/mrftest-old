#include "ai/hl/hl.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include <iostream>
#include <gtkmm.h>

using namespace AI::HL;
using namespace AI::HL::W;

namespace {
	class KalmanIntersectTestFactory : public HighLevelFactory {
		public:
			KalmanIntersectTestFactory() : HighLevelFactory("Hawdy, Kalman Intersect Test") {
			}

			HighLevel::Ptr create_high_level(World &world) const;
	};

	KalmanIntersectTestFactory factory_instance;

	class KalmanIntersectTest : public HighLevel {
		public:
			KalmanIntersectTest(World &world) : world(world), isect_robot_dst(0,0) {
				reset_btn.set_label("Reset");
				reset_btn.signal_clicked().connect( sigc::mem_fun(*this, &KalmanIntersectTest::on_reset_btn_clicked) );
				ui_box.add( reset_btn );
			}

			KalmanIntersectTestFactory &factory() const {
				return factory_instance;
			}

			void tick() {
				// sample ball position once per tick
				Point new_path_point = world.ball().position();
				path_points.push_back( new_path_point );
				//std::cout << "(" << new_path_point.x << ", " << new_path_point.y <<")\n";
				Point ball_velocity =  world.ball().velocity();
				// this position should take into account of the velocity of the robot and ball PROPERLY, subject to change
				isect_robot_dst = new_path_point + ball_velocity * ball_velocity.len();
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
			Gtk::VBox ui_box;
			std::vector<Point> path_points; 
			Point isect_robot_dst;
			
			
			void on_reset_btn_clicked(){
				path_points.clear();
				isect_robot_dst = Point(0,0);
			}
	};

	HighLevel::Ptr KalmanIntersectTestFactory::create_high_level(World &world) const {
		HighLevel::Ptr p(new KalmanIntersectTest(world));
		return p;
	}
}

