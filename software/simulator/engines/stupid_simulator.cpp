#include "geom/angle.h"
#include "simulator/engine.h"
#include "world/timestep.h"
#include <vector>
#include <glibmm/refptr.h>
#include <libxml++/libxml++.h>
#include <ode/ode.h>
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
	// A collisionless-kinematic ball_impl.
	//
	class ck_ball : public virtual ball_impl {
		public:
			typedef Glib::RefPtr<ck_ball> ptr;

			ck_ball() : the_position(0.0, 0.1), the_velocity(0.0, 0.0) {
			}

			void kick(const point &direction) {
				the_velocity += direction;
			}

			void update() {
				the_position += velocity() / static_cast<double>(TIMESTEPS_PER_SECOND);
				const point &velocity_diff = acceleration() / static_cast<double>(TIMESTEPS_PER_SECOND);
				if (velocity_diff.len() >= the_velocity.len())
					the_velocity = point(0.0, 0.0);
				else
					the_velocity += velocity_diff;

				add_position(the_position);
			}

			virtual point position() const {
				return the_position;
			}

			virtual point velocity() const {
				return the_velocity;
			}

			virtual point acceleration() const {
				if (the_velocity.len() > EPS) {
					return -the_velocity / the_velocity.len() * BALL_DECELERATION;
				} else {
					return point(0.0, 0.0);
				}
			}

		private:
			point the_position, the_velocity;
	};

	//
	// A collisionless-kinematic player_impl.
	//
	class ck_player : public virtual player_impl {
		public:
			typedef Glib::RefPtr<ck_player> ptr;

			ck_player() : the_position(0.0, 0.0), the_velocity(0.0, 0.0), target_velocity(0.0, 0.0), the_orientation(0.0), avelocity(0.0) {
			}

			void update() {
				the_position += the_velocity;
				const point &diff = target_velocity - the_velocity;
				if (diff.len() < BOT_MAX_ACCELERATION / TIMESTEPS_PER_SECOND) {
					the_velocity = target_velocity;
				} else {
					the_velocity += diff / diff.len() * BOT_MAX_ACCELERATION / static_cast<double>(TIMESTEPS_PER_SECOND);
				}

				the_orientation = angle_mod(the_orientation + avelocity);

				add_position(the_position);
			}

			virtual point position() const {
				return the_position;
			}

			virtual double orientation() const {
				return the_orientation;
			}

			virtual bool has_ball() const {
				return false;
			}

			virtual void move_impl(const point &vel, double avel) {
				target_velocity = vel;
				avelocity = avel;
			}

			virtual void dribble(double) {
			}

			virtual void kick(double) {
			}

			virtual void chip(double) {
			}

		private:
			point the_position, the_velocity, target_velocity;
			double the_orientation, avelocity;
	};

	//
	// A collisionless-kinematic simulator_engine.
	//
	class ck_engine : public virtual simulator_engine {
		public:
			dWorldID eworld;
			dSpaceID space;
			dGeomID ground;


			ck_engine() : the_ball(new ck_ball) {
			eworld = dWorldCreate(); 
			dWorldSetGravity (eworld,0,0.0,-9.81);

			space = dHashSpaceCreate (0);
  
  			dWorldSetGravity (eworld,0,0,-0.5);
  			ground = dCreatePlane (space,0,0,1,0);
    			dWorldSetContactSurfaceLayer(eworld, 0.001);

			}

			virtual void update() {
				the_ball->update();
				for (unsigned int i = 0; i < the_players.size(); i++)
					the_players[i]->update();
			}
			virtual void setWorld(dWorldID *world) {
				eworld = *world;
			}
			virtual ball_impl::ptr get_ball() {
				return the_ball;
			}

			virtual player_impl::ptr add_player() {
				dBodyID b = dBodyCreate (eworld);

				ck_player::ptr p(new ck_player);
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
			ck_ball::ptr the_ball;
			std::vector<ck_player::ptr> the_players;
	};

	//
	// A factory for creating ck_engines.
	//
	class ck_engine_factory : public virtual simulator_engine_factory {
		public:
			ck_engine_factory() : simulator_engine_factory("Stupid Crappy Simulator") {
			}

			virtual simulator_engine::ptr create_engine(xmlpp::Element *) {
				simulator_engine::ptr p(new ck_engine);
				return p;
			}
	};

	//
	// The global instance of ck_engine_factory.
	//
	ck_engine_factory fact;

	simulator_engine_factory &ck_engine::get_factory() {
		return fact;
	}

	// returns resultant velocities after an elastic collision
	// v1,v2 are original velocities, m1,m2 are object masses
	// dir_orig is difference between positions of the two objects
	std::pair<point,point> elastic_collision(const point& v1, double m1, const point& v2, double m2, const point& dir_orig){
		point dir = dir_orig / dir_orig.len();
		double vpar1 = (v1.dot(dir));
		double vpar2 = (v2.dot(dir));
		std::pair<point,point> vret;
		vret.first = (((m1-m2)*vpar1 + (2*m2)*vpar2)/(m1+m2)) * dir + (v1 - vpar1*dir);
		vret.second = (((2*m1)*vpar1 - (m1-m2)*vpar2)/(m1+m2)) * dir + (v2 - vpar2*dir);
		return vret;
	}
}

