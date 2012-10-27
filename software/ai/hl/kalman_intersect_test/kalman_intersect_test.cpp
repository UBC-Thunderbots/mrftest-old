#include "ai/hl/hl.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include <gtkmm/adjustment.h>
#include <gtkmm/box.h>
#include <gtkmm/button.h>
#include <gtkmm/label.h>
#include <gtkmm/scrollbar.h>

using namespace AI::HL;
using namespace AI::HL::W;

namespace {
	const int NUMBER_OF_TARGETS = 2;
	Point TARGETS[NUMBER_OF_TARGETS] = { Point(0.0, -2.0), Point(0.0, 2.0) };
	Angle TARGETS_ORIENT[NUMBER_OF_TARGETS] = { Angle::of_radians(0.5 * M_PI), Angle::of_radians(-0.5 * M_PI) };

	class KalmanIntersectTest : public HighLevel {
		public:
			KalmanIntersectTest(World world) : world(world), lbl_xposition("XPosition"), adj_xposition(0.0, -4.0, 4.0, 0.1, 1.0), hsb_xposition(adj_xposition), lbl_yposition("YPosition"), adj_yposition(0.0, -2.0, 2.0, 0.1, 0.5), hsb_yposition(adj_yposition), lbl_oposition("OPosition"), adj_oposition(0.0, 0.0, 2 * M_PI, 0.1 * M_PI, 0.5 * M_PI), hsb_oposition(adj_oposition), lbl_kick_power("KPower"), adj_kick_power(0.0, 0.0, 10.0, 0.5, 1.0), hsb_kick_power(adj_kick_power), isect_robot_dst(0, 0), xposition(0.0), yposition(0.0), oposition(Angle::ZERO), to_run(false), to_dribble(false), to_kick(false) {
				adj_xposition.signal_value_changed().connect(sigc::mem_fun(*this, &KalmanIntersectTest::on_xposition_value_changed));
				ui_box.add(lbl_xposition);
				ui_box.add(hsb_xposition);
				adj_yposition.signal_value_changed().connect(sigc::mem_fun(*this, &KalmanIntersectTest::on_yposition_value_changed));
				ui_box.add(lbl_yposition);
				ui_box.add(hsb_yposition);
				adj_oposition.signal_value_changed().connect(sigc::mem_fun(*this, &KalmanIntersectTest::on_oposition_value_changed));
				ui_box.add(lbl_oposition);
				ui_box.add(hsb_oposition);
				ui_box.add(lbl_kick_power);
				ui_box.add(hsb_kick_power);


				dribble_btn.set_label("dribble");
				dribble_btn.signal_clicked().connect(sigc::mem_fun(*this, &KalmanIntersectTest::on_dribble_btn_clicked));

				kick_btn.set_label("kick");
				kick_btn.signal_clicked().connect(sigc::mem_fun(*this, &KalmanIntersectTest::on_kick_btn_clicked));

				run_btn.set_label("Run bot");
				run_btn.signal_clicked().connect(sigc::mem_fun(*this, &KalmanIntersectTest::on_run_btn_clicked));

				reset_btn.set_label("Reset");
				reset_btn.signal_clicked().connect(sigc::mem_fun(*this, &KalmanIntersectTest::on_reset_btn_clicked));
				ui_box.add(reset_btn);
				ui_box.add(run_btn);
				ui_box.add(kick_btn);
				ui_box.add(dribble_btn);
			}

			HighLevelFactory &factory() const;

			void tick() {
				// sample ball position once per tick
				Point new_path_point = world.ball().position();
				// path_points.push_back( new_path_point );
				// std::cout << "(" << new_path_point.x << ", " << new_path_point.y <<")\n";
				Point ball_velocity = world.ball().velocity();
				// this position should take into account of the velocity of the robot and ball PROPERLY, subject to change
				isect_robot_dst = new_path_point + ball_velocity *ball_velocity.len();

				// enable the bot to go to the next dst

				if (world.friendly_team().size() == 1 && to_run) {
					Player player = world.friendly_team().get(0);
					player.move(Point(xposition, yposition), oposition, Point());
					player.type(AI::Flags::MoveType::PIVOT);
					if (to_dribble) {
						player.type(AI::Flags::MoveType::DRIBBLE);
					} else {
						player.type(AI::Flags::MoveType::NORMAL);
					}
					if (to_kick) {
						player.kick(hsb_kick_power.get_value());
						to_kick = false;
					}
				}
			}

			Gtk::Widget *ui_controls() {
				return &ui_box;
			}

			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) {
				ctx->set_source_rgb(1, 0.5, 0.5);
				if (!path_points.empty()) {
					ctx->move_to(path_points.begin()->x, path_points.begin()->y);
				}
				for (std::vector<Point>::iterator iter = path_points.begin(); iter < path_points.end(); ++iter) {
					ctx->line_to((*iter).x, (*iter).y);
				}
				ctx->set_line_width(0.05);
				ctx->stroke();

				ctx->set_source_rgb(1.0, 1.0, 0);
				ctx->arc(xposition, yposition, 0.3, oposition.to_radians() + 0.2 * M_PI, oposition.to_radians() - 0.2 * M_PI);
				ctx->stroke();
			}

		private:
			World world;
			// may expand this structure to include time stamp
			Gtk::Button reset_btn;
			Gtk::Button run_btn;
			Gtk::Button dribble_btn;
			Gtk::Button kick_btn;
			Gtk::Label lbl_xposition;
			Gtk::Adjustment adj_xposition;
			Gtk::HScrollbar hsb_xposition;
			Gtk::Label lbl_yposition;
			Gtk::Adjustment adj_yposition;
			Gtk::HScrollbar hsb_yposition;
			Gtk::Label lbl_oposition;
			Gtk::Adjustment adj_oposition;
			Gtk::HScrollbar hsb_oposition;
			Gtk::Label lbl_kick_power;
			Gtk::Adjustment adj_kick_power;
			Gtk::HScrollbar hsb_kick_power;
			Gtk::VBox ui_box;
			std::vector<Point> path_points;
			Point isect_robot_dst;

			double xposition;
			double yposition;
			Angle oposition;

			bool to_run;
			bool to_dribble;
			bool to_kick;

			void on_xposition_value_changed() {
				xposition = hsb_xposition.get_value();
			}

			void on_yposition_value_changed() {
				yposition = hsb_yposition.get_value();
			}

			void on_oposition_value_changed() {
				oposition = Angle::of_radians(hsb_oposition.get_value());
			}

			void on_reset_btn_clicked() {
				path_points.clear();
				isect_robot_dst = Point(0, 0);
			}

			void on_run_btn_clicked() {
				if (!to_run) {
					to_run = true;
					run_btn.set_label("Stop bot");
				} else {
					to_run = false;
					run_btn.set_label("Run bot");
				}
			}

			void on_dribble_btn_clicked() {
				to_dribble = !to_dribble;
			}

			void on_kick_btn_clicked() {
				to_kick = true;
			}
	};
}

HIGH_LEVEL_REGISTER(KalmanIntersectTest)

