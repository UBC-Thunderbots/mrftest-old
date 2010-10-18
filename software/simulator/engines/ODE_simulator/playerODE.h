#ifndef SIMULATOR_ENGINES_ODE_SIMULATOR_PLAYERODE_H
#define SIMULATOR_ENGINES_ODE_SIMULATOR_PLAYERODE_H

#include "simulator/player.h"
#include <ode/ode.h>



/**
 * The back-end behind an ODE SimulatorPlayer object.
 */
class PlayerODE : public SimulatorPlayer {
	public:
		double x_len;
		double y_len;
		double momentInertia;

		typedef RefPtr<PlayerODE> Ptr;

	private:
		/**
		 * The rectangular geometry for the front collision.
		 */
		dGeomID robotGeomTop;

		/**
		 * The Cylindrical geometry for most collisions.
		 */
		dGeomID robotGeomTopCyl;

		/**
		 * This is not used.
		 */
		dGeomID dribbleArmL;

		/**
		 * This is not used.
		 */
		dGeomID dribbleArmR;

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

		dVector3 *Vertices;
		unsigned int *Triangles;
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
		bool hasContactPenetration(dVector3 pos);

		bool hasContactWithFace(dVector3 pos);
		/**
		 * Called when we find a robot-ball collision.
		 * May do some additional testing beyond this to make sure "has ball".
		 */
		void set_has_ball();

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

	public:
		void position(const Point &pos);

		void velocity(const Point &vel);

		void orientation(double orient);

		void avelocity(double avel);

		dTriMeshDataID create_robot_geom();
};

#endif

