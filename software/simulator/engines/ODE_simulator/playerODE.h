#ifndef SIMULATOR_ENGINES_ODE_SIMULATOR_PLAYERODE_H
#define SIMULATOR_ENGINES_ODE_SIMULATOR_PLAYERODE_H

#include "simulator/player.h"
// #include "player_geom.h"
#include "compo_player_geom.h"
#include <ode/ode.h>



/**
 * The back-end behind an ODE SimulatorPlayer object.
 */
class PlayerODE : public Simulator::Player {
	public:
		double x_len;
		double y_len;
		double momentInertia;

		typedef RefPtr<PlayerODE> Ptr;
		Compo_player_geom p_geom;

	private:
		/**
		 * We need to interact with the simulator world so store its ID here.
		 */
		dWorldID world;

		/**
		 * The ID for the robot's body in the simulator.
		 */
		dBodyID body;

		/**
		 * The mass object for the robot (keeps track of things like inertial
		 * moments).
		 */
		dMass mass;

		/**
		 * Whether we are moving the robot.
		 */
		bool posSet;

		bool player_has_ball;

		/**
		 * I don't know why we keep track of the ball ID, oh right the retarded
		 * hasball routine Number 1.
		 */
		dGeomID ballGeom;
		double updates_per_tick;

		Point *wheel_position;
		Point *force_direction;

	public:
		PlayerODE(dWorldID dworld, dSpaceID dspace, dGeomID ballGeom, double ups_per_tick);
		~PlayerODE();

		double get_height() const;

		void update();

		Point position() const;

		double orientation() const;

		bool has_ball() const;

		unsigned int dribbler_speed() const;

		bool robot_contains_shape(dGeomID geom);

	public:
		void pre_tic(double TimeStep);

		bool has_kick_set() {
			return orders.kick;
		}

	private:
		bool execute_kick();

	public:
		bool has_chip_set() {
			return orders.chip;
		}

	private:
		bool execute_chip();

		void dribble(double set_point);

	public:
		void position(const Point &pos);

		void velocity(const Point &vel);

		void orientation(double orient);

		void avelocity(double avel);
};

#endif

