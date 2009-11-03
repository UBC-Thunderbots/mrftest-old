#include "geom/angle.h"
#include "simulator/engine.h"
#include "world/timestep.h"
#include <vector>
#include <glibmm/refptr.h>
#include <libxml++/libxml++.h>
#include <ode/ode.h>
#include "playerODE.h"
#include "ballODE.h"
namespace {
	//
	// The limit of floating-point precision.
	//
	const double EPS = 1.0e-9;

	//
	// The maximum acceleration of a robot, in metres per second squared.
	//
	const double BOT_MAX_ACCELERATION = 10.0;

	//
	// The maximum velocity of a robot, in metres per second.
	//
	const double BOT_MAX_VELOCITY = 5.0;

	//
	// The acceleration due to friction against the ball, in metres per second squared.
	//
	const double BALL_DECELERATION = 6.0;

	//
	// The force of gravity N/kg
	//
	const double GRAVITY = -9.81;

	//
	// A simulator_engine.
	//
	class sim_engine : public virtual simulator_engine {
		public:
			dWorldID eworld;
			dSpaceID space;
			dGeomID ground;


			sim_engine(){
				eworld = dWorldCreate(); 
				dWorldSetGravity (eworld,0,0.0,GRAVITY);

				space = dHashSpaceCreate (0);

  				ground = dCreatePlane (space,0,0,1,0);
    				dWorldSetContactSurfaceLayer(eworld, 0.001);


				ballODE::ptr b(new ballODE(eworld));
				the_ball = b;
			}

			virtual void update() {
				
			}
			virtual void setWorld(dWorldID world) {
				eworld = world;
			}
			virtual ball_impl::ptr get_ball() {
				
				return the_ball;
			}

			virtual player_impl::ptr add_player() {
				playerODE::ptr p(new playerODE(eworld));
				the_players.push_back(p);
				return p;
			}

			virtual void remove_player(player_impl::ptr p) {
				for (unsigned int i = 0; i < the_players.size(); i++) {
					if (player_impl::ptr::cast_static(the_players[i]) == p) {
						the_players.erase(the_players.begin() + i);
						return;
					}
				}
			}

			virtual Gtk::Widget *get_ui_controls() {
				return 0;
			}

			virtual simulator_engine_factory &get_factory();

		private:
			ballODE::ptr the_ball;
			std::vector<playerODE::ptr> the_players;
	};

	//
	// A factory for creating sim_engines.
	//
	class sim_engine_factory : public virtual simulator_engine_factory {
		public:
			sim_engine_factory() : simulator_engine_factory("Open Dynamics Engine Simulator") {
			}

			virtual simulator_engine::ptr create_engine(xmlpp::Element *) {
				simulator_engine::ptr p(new sim_engine);
				return p;
			}
	};

	//
	// The global instance of sim_engine_factory.
	//
	sim_engine_factory fact;

	simulator_engine_factory &sim_engine::get_factory() {
		return fact;
	}
}

