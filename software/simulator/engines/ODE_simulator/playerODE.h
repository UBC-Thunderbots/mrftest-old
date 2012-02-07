#ifndef SIMULATOR_ENGINES_ODE_SIMULATOR_PLAYERODE_H
#define SIMULATOR_ENGINES_ODE_SIMULATOR_PLAYERODE_H

#include "simulator/player.h"
// #include "player_geom.h"
#include "compo_player_geom.h"
#include "util/fd.h"
#include <ode/ode.h>



/**
 * The back-end behind an ODE SimulatorPlayer object.
 */
class PlayerODE : public Simulator::Player {
	public:
		dReal momentInertia;

		CompoPlayerGeom p_geom;

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

		/**
		 * I don't know why we keep track of the ball ID, oh right the retarded
		 * hasball routine Number 1.
		 */
		dGeomID ballGeom;
		dReal updates_per_tick;

		Point wheel_position[4];
		Point force_direction[4];

	public:
		/**
		 * Constructor method for the robot model contained in the simulator
		 */
		PlayerODE(dWorldID dworld, dSpaceID dspace, dGeomID ballGeom, dReal ups_per_tick);

		/**
		 * Accessor to get the height of the middle of the robot (should be ROBOT_HEIGHT/2)
		 */
		dReal get_height() const;

		void update();

		Point position() const;

		Angle orientation() const;

		bool has_ball() const;

		unsigned int dribbler_speed() const;

		bool robot_contains_shape(dGeomID geom);

	public:
		void pre_tic(dReal TimeStep);

		bool has_kick_set() const {
			return orders.kick;
		}

	private:
		bool execute_kick();

	public:
		bool has_chip_set() const {
			return orders.chip;
		}

	private:
		bool execute_chip();

		void dribble(dReal set_point);

	public:
		void position(const Point &pos);

		void velocity(const Point &vel);

		void orientation(Angle orient);

		void avelocity(double avel);

		void load_state(const FileDescriptor &fd);

		void save_state(const FileDescriptor &fd) const;
};

#endif

