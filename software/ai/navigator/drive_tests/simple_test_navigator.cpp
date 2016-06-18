#include "ai/navigator/navigator.h"
#include "ai/navigator/rrt_planner.h"
#include "geom/angle.h"
#include "util/dprint.h"
#include "ai/hl/stp/param.h"
#include "ai/navigator/util.h"
#include "util/main_loop.h"
#include <iostream>
#include <functional>
#include <gtkmm.h>
#include <glibmm/ustring.h>


#define SOME_TINY_NUMBER 1e-3

using AI::Nav::Navigator;
using AI::Nav::NavigatorFactory;
using namespace AI::Nav::W;
using namespace AI::Nav::Util;
using namespace AI::Flags;
using namespace Glib;
using namespace std;


namespace AI {
	namespace Nav {
		namespace SimpleT {
		static const string CHOOSE_TEST_TEXT = "<CHOOSE_TEST>";

		/*
		 * A navigator that tests the functions of movement primitives.
		 */
		class SimpleTest final : public Navigator {

		public:
			struct Prim_Test;
			typedef void (Prim_Test::*test_function)(Player);

			void tick() override;
			static Angle getRelativeAngle(Point robotPos, Angle robotOri, Point dest);
			static Point getRelativeVelocity(Point robotPos, Angle robotOri, Point dest);
			void onComboChanged();
			void onTestComboChanged();

			explicit SimpleTest(World world);
			NavigatorFactory &factory() const override;

			// The current active movement primitive test set
			Prim_Test* currentTest = new Prim_Test();

			// A map of displayed names to primitive test sets
			map<string , Prim_Test*> primitiveMap = {};

			Gtk::Widget *ui_controls() override;
			Gtk::VBox vbox;
			Gtk::ComboBoxText combo;
			Gtk::ComboBoxText testCombo;
			//Gtk::Widget *primTestControls;




			//parent struct for Test
			struct Prim_Test {
			public:
				map<string, test_function> testsMap;
				test_function currTestFunc;
				Gtk::VBox box;
				Gtk::Button activate;
				Player player;

				virtual Gtk::Widget &returnGtkWidget(){
					return box;
				}
				void callTestFunction(){
					if (currTestFunc != NULL){
						(this->*currTestFunc)(player);
					}
				}
				void doNothing(Player player){

				}
				Prim_Test() {
					currTestFunc = &Prim_Test::doNothing;
					testsMap[CHOOSE_TEST_TEXT] = &Prim_Test::doNothing;
					activate.set_label("Run");
					box.pack_end(activate);
					activate.signal_clicked().connect(sigc::mem_fun(this,&Prim_Test::callTestFunction));
					//testsMap = new map<string,std::function<void(Prim_Test*,Player)>>;
				}
			};

			struct Move_Shoot_Test: Prim_Test {

				//public:
					Point dest;
					double POWERRRR;
					bool chip;
					Angle orient;
					string NAME = "Move_Shoot";
					World world;
					bool hasShot = false;

					//Gtk::VBox box;
					Gtk::HScale point_x_slider;
					Gtk::HScale point_y_slider;
					Gtk::SpinButton powerEntry;
					Gtk::SpinButton angleEntry;
					Gtk::CheckButton toChip;

					Gtk::Label xLbl;
					Gtk::Label yLbl;
					Gtk::Label powerLbl;
					Gtk::Label angleLbl;

				//GUI Stuff
				//Gtk::Widget point_x_slider =
				//gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL,-world.field().length()/2,world.field().length()/2);

				Move_Shoot_Test(World w): Prim_Test(), dest(Point(0,0)), POWERRRR(0), chip(false), orient(Angle::of_degrees(0)),world(w) {
					testsMap["Shoot"] = static_cast<test_function>(&Move_Shoot_Test::testShoot);
					testsMap["ShootOrient"] = static_cast<test_function>(&Move_Shoot_Test::testShootOrient);
					buildGtkWidget();
				}

				/*
				Move_Shoot_Test( Point d, double p, bool c, Angle a) :
						dest(d), POWERRRR(p), chip(c), orient(a) {
					testsMap["Shoot"] = static_cast<test_function>(&Move_Shoot_Test::testShoot);
					testsMap["ShootOrient"] = static_cast<test_function>(&Move_Shoot_Test::testShootOrient);
				}
	*/
				void testShoot(Player player){
					player.move_shoot(dest, POWERRRR, chip);

				}

				void testShootOrient(Player player){
					player.move_shoot(dest,orient,POWERRRR,chip);

				}

				void buildGtkWidget(){
					point_x_slider.set_range(-world.field().length()/2,world.field().length()/2);
					point_y_slider.set_range(-world.field().width()/2,world.field().width()/2);

					angleEntry.set_range(-360,360);
					powerEntry.set_range(0,50000);

					//point_x_slider.set_name("Field X",false);
					//point_y_slider.set_name("Field Y",false);

					toChip.set_label("Chip");

					xLbl.set_label("X Coordinate");
					yLbl.set_label("Y Coordinate");
					angleLbl.set_label("Angle (degrees)");
					powerLbl.set_label("Power");

					//add widgets to vbox
					box.add(xLbl);
					box.add(point_x_slider); //x for dest

					box.add(yLbl);
					box.add(point_y_slider); //y for dest

					box.add(toChip); //chip

					box.add(angleLbl);
					box.add(angleEntry); //orient

					box.add(powerLbl);
					box.add(powerEntry); //POWERRRR

					box.show_all();

					//link signals
					point_x_slider.signal_value_changed().connect(sigc::mem_fun(this,&Move_Shoot_Test::onPointXChanged));
					point_y_slider.signal_value_changed().connect(sigc::mem_fun(this,&Move_Shoot_Test::onPointYChanged));
					toChip.signal_clicked().connect(sigc::mem_fun(this,&Move_Shoot_Test::onChipChanged));
					angleEntry.signal_value_changed().connect(sigc::mem_fun(this,&Move_Shoot_Test::onAngleChanged));
					powerEntry.signal_value_changed().connect(sigc::mem_fun(this,&Move_Shoot_Test::onPowerChanged));
				}

				void onPointXChanged(){
					dest = Point(point_x_slider.get_value(),dest.y);

				}
				void onPointYChanged(){
					dest = Point(dest.x,point_y_slider.get_value());

				}

				void onChipChanged(){
					chip = toChip.get_active();
				}

				void onAngleChanged(){
					orient = Angle::of_degrees(angleEntry.get_value());
				}
				void onPowerChanged(){
					POWERRRR = powerEntry.get_value();
				}

				Gtk::Widget &returnGtkWidget(void) override{
					return box;
				}

			};

			struct Move_Move_Test: Prim_Test {
				Point dest;
				Angle orient;
				double time_delta;
				World world;

				//Gtk::VBox box;
				Gtk::HScale point_x_slider;
				Gtk::HScale point_y_slider;
				Gtk::SpinButton timeEntry;
				Gtk::SpinButton angleEntry;


				Gtk::Label xLbl;
				Gtk::Label yLbl;
				Gtk::Label timeLbl;
				Gtk::Label angleLbl;

				const string NAME = "Move_Move";
				Move_Move_Test(World w): Prim_Test(), dest(Point(0,0)), orient(Angle::of_degrees(45)), time_delta(0), world(w){
					testsMap["Move"] = static_cast<test_function>(&Move_Move_Test::testMoveToDestination);
					testsMap["MoveOrient"] = static_cast<test_function>(&Move_Move_Test::testMoveOrientToDestination);
					testsMap["MoveTimeDelta"] = static_cast<test_function>(&Move_Move_Test::testMoveTimeDeltaToDestination);
					testsMap["MoveOrientTimeDelta"] = static_cast<test_function>(&Move_Move_Test::testMoveOrientTimeDeltaToDestination);
					testsMap["MoveToBall"] = static_cast<test_function>(&Move_Move_Test::testMoveToBall);
					buildGtkWidget();
				}

				/*
				Move_Move_Test( Point d, Angle a, double t) :
						dest(d), orient(a), time_delta(t) {
					testsMap["Move"] = static_cast<test_function>(&Move_Move_Test::testMoveToDestination);
					testsMap["MoveOrient"] = static_cast<test_function>(&Move_Move_Test::testMoveOrientToDestination);
					testsMap["MoveTimeDelta"] = static_cast<test_function>(&Move_Move_Test::testMoveTimeDeltaToDestination);
					testsMap["MoveOrientTimeDelta"] = static_cast<test_function>(&Move_Move_Test::testMoveOrientTimeDeltaToDestination);

				}
	*/
				void testMoveToDestination(Player player){
					player.move_move(dest);
				}

				void testMoveOrientToDestination(Player player){
					player.move_move(dest, orient);
				}

				void testMoveTimeDeltaToDestination(Player player){
					player.move_move(dest, time_delta);
				}

				void testMoveOrientTimeDeltaToDestination(Player player){
					player.move_move(dest, orient,time_delta);
				}

				void testMoveToBall(Player player){
					Point robot_local_dest = getRelativeVelocity(player.position(), player.orientation(), world.ball().position());
					Angle robot_local_angle = getRelativeAngle(player.position(), player.orientation(), world.ball().position());
					LOG_INFO(Glib::ustring::compose(u8"pos delta = %1", robot_local_dest));
					player.move_move(robot_local_dest, robot_local_angle);
				}

				void buildGtkWidget(){
					LOG_INFO(Glib::ustring::compose(u8"%1", world.field().length()));
					point_x_slider.set_range(-world.field().length()/2,world.field().length()/2);
					point_y_slider.set_range(-world.field().width()/2,world.field().width()/2);
					angleEntry.set_range(-360,360); //change range? 360 degrees sounds excessive
					timeEntry.set_range(-100,100); //dunno

					xLbl.set_label("X Coordinate");
					yLbl.set_label("Y Coordinate");
					angleLbl.set_label("Angle (degrees)");
					timeLbl.set_label("Time Delta");

					//add widgets to vbox
					box.add(xLbl);
					box.add(point_x_slider); //x for dest

					box.add(yLbl);
					box.add(point_y_slider); //y for dest


					box.add(angleLbl);
					box.add(angleEntry); //orient

					box.add(timeLbl);
					box.add(timeEntry); //POWERRRR

					box.show_all();

					//link signals
					point_x_slider.signal_value_changed().connect(sigc::mem_fun(this,&Move_Move_Test::onPointXChanged));
					point_y_slider.signal_value_changed().connect(sigc::mem_fun(this,&Move_Move_Test::onPointYChanged));
					angleEntry.signal_value_changed().connect(sigc::mem_fun(this,&Move_Move_Test::onAngleChanged));
					timeEntry.signal_value_changed().connect(sigc::mem_fun(this,&Move_Move_Test::onTimeChanged));
				}

				void onPointXChanged(){
					dest = Point(point_x_slider.get_value(), dest.y);

				}
				void onPointYChanged(){
					dest = Point(dest.x, point_y_slider.get_value());

				}

				void onAngleChanged(){
					orient = Angle::of_degrees(angleEntry.get_value());
				}
				void onTimeChanged(){
					time_delta = timeEntry.get_value();
				}

				Gtk::Widget &returnGtkWidget(void) override{
					return box;
				}


			};

			/*
			struct Move_Move_Integration : Prim_Test{
				World world;
				Point dest;
				Angle orient;
				double time_delta;
				const string NAME = "Move_Move";
				Move_Move_Integration():world(world),dest(Point(0,0)),orient(Angle::of_degrees(45)),time_delta(0){

					testsMap["MoveToBall"] = static_cast<test_function>(&Move_Move_Integration::testMoveToBall);
				}

				void testMoveToBall(Player player){
					Point robot_local_dest = getRelativeVelocity(player.position(), player.orientation(), world.ball().position());
					Angle robot_local_angle = getRelativeAngle(player.position(), player.orientation(), world.ball().position());
					player.move_move(robot_local_dest, robot_local_angle);
				}
			};
			*/
			struct Move_Dribble_Test: Prim_Test {
				Point dest;
				Angle orient;
				bool small_kick_allowed;
				double rpm;
				string NAME = "Move_Dribble";
				Move_Dribble_Test():dest(Point(0,0)),orient(Angle::of_degrees(45)),small_kick_allowed(true),rpm(0){
					testsMap["Dribble"] = static_cast<test_function>(&Move_Dribble_Test::testDribble);
				}

				Move_Dribble_Test( Point d, Angle a, double r, bool s) :
						dest(d), orient(a), small_kick_allowed(s), rpm(r) {
					testsMap["Dribble"] = static_cast<test_function>(&Move_Dribble_Test::testDribble);
				}

				void testDribble(Player player){
					player.move_dribble(dest,orient,rpm,small_kick_allowed);
				}

			};

			struct Move_Pivot_Test: Prim_Test {
				Point centre;
				Angle swing;
				Angle orient;
			public:
				string NAME = "Move_Pivot";

				Move_Pivot_Test():centre(Point(0,0)),swing(Angle::of_degrees(45)),orient(Angle::of_degrees(45)){
					testsMap["Pivot"] = static_cast<test_function>(&Move_Pivot_Test::testPivot);
				}

				Move_Pivot_Test( Point c, Angle s, Angle a) :
						centre(c), swing(s), orient(a) {
					testsMap["Pivot"] = static_cast<test_function>(&Move_Pivot_Test::testPivot);
				}

				void testPivot(Player player){
					player.move_pivot(centre,swing,orient);
				}
				//vbox.add(primitiveMap[combo.

			};

			struct Move_Spin_Test: Prim_Test {
				Point dest;
				Angle speed;

				Move_Spin_Test():dest(Point(0,0)),speed(Angle::of_degrees(0)){
					testsMap["Spin"] = static_cast<test_function>(&Move_Spin_Test::testSpin);
				}

				Move_Spin_Test( Point d, Angle s) :
						dest(d), speed(s) {
					testsMap["Spin"] = static_cast<test_function>(&Move_Spin_Test::testSpin);
				}

				void testSpin(Player player){
					player.move_spin(dest,speed);
				}

			};

			struct Bezier_Test:Prim_Test{
				//quadratic
				Point p0;
				Point p1;
				Point p2;
				double t;

				Bezier_Test():p0(Point(0,0)),p1(Point(0,0)),p2(Point(0,0)),t(0){

				}

				Bezier_Test(Point p0,Point p1,Point p2, double t):
					p0(p0),p1(p1),p2(p2), t(t){
				}


			};

		private:
			const Glib::ustring CHOOSE_PLAY_TEXT = u8"<Choose Play>";
			void buildTestGUI();
			//Point start;
			//bool called = false;

		};

		}
	}
}

using AI::Nav::SimpleT::SimpleTest;

SimpleTest::SimpleTest(AI::Nav::W::World world) :
		Navigator(world) {
	//Point start = world.friendly_team()[0].position();
	//primitiveMap->insert(pair<string,Prim_Test>(NAME,));

	buildTestGUI();
}

void SimpleTest::buildTestGUI(){
	combo.append(CHOOSE_PLAY_TEXT);

	primitiveMap["Move_Spin"] = new Move_Spin_Test();
	primitiveMap["Move_Move"] = new Move_Move_Test(world);
	primitiveMap["Move_Shoot"] = new Move_Shoot_Test(world);
	primitiveMap["Move_Dribble"] = new Move_Dribble_Test();
	primitiveMap["Move_Pivot"] = new Move_Pivot_Test();

	//Move_Shoot_Test* mTest = new Move_Shoot_Test(world);

	for(auto const &i : primitiveMap){
		//vbox.add(primitiveMap[combo.
		combo.append(Glib::ustring(i.first));
	}

	combo.set_active_text(CHOOSE_PLAY_TEXT);
	vbox.add(combo);

	vbox.add(testCombo);

	combo.signal_changed().connect(sigc::mem_fun(this,&SimpleTest::onComboChanged));
	testCombo.signal_changed().connect(sigc::mem_fun(this,&SimpleTest::onTestComboChanged));
	vbox.add(currentTest->returnGtkWidget());
}

void SimpleTest::tick() {
	//select first available player
	//vbox.add(primitiveMap[combo.get_active_text()].ui_controls());
	//printf("start");
	currentTest->player = world.friendly_team()[0];
	//printf("yep");
	//currentTest->callTestFunction(player);
	/*
	Move_Move_Test *tf = static_cast<Move_Move_Test*>(primitiveMap["Move_Move"]);
	tf->testMoveOrientTimeDeltaToDestination(player);
	*/

}

Gtk::Widget *SimpleTest::ui_controls() {
	return &vbox;
}

void SimpleTest::onComboChanged(){
	vbox.remove(currentTest->returnGtkWidget());

	currentTest = primitiveMap[combo.get_active_text()];

	testCombo.remove_all();
	for(auto const &i : currentTest->testsMap){
		//vbox.add(primitiveMap[combo.
		testCombo.append(Glib::ustring(i.first));
	}
	testCombo.set_active_text(CHOOSE_TEST_TEXT);

	vbox.add(currentTest->returnGtkWidget());

	//LOG_INFO(combo.get_active_text());
}
void SimpleTest::onTestComboChanged(){
	LOG_INFO(testCombo.get_active_text());
	// If we can't find the
	if(currentTest->testsMap.find(testCombo.get_active_text()) == currentTest->testsMap.end()){
		currentTest->currTestFunc = currentTest->testsMap[CHOOSE_TEST_TEXT];
	}
	else{
		currentTest->currTestFunc = currentTest->testsMap[testCombo.get_active_text()];
	}
}
Angle SimpleTest::getRelativeAngle(Point robotPos, Angle robotOri, Point dest){
	Point pos_diff = dest-robotPos;
	//Point robot_local_dest = pos_diff.rotate(-robotOri).norm(0.1);
	Angle robot_local_angle = pos_diff.orientation()-robotOri;
	return robot_local_angle;
}

Point SimpleTest::getRelativeVelocity(Point robotPos, Angle robotOri, Point dest){
	Point pos_diff = dest-robotPos;
	Point robot_local_dest = pos_diff.rotate(-robotOri).norm(0.2);
	//Angle robot_local_angle = (dest - robotPos).orientation()-robotOri;
	return robot_local_dest;
}

NAVIGATOR_REGISTER(SimpleTest)

