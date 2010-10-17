#ifndef SIMULATOR_ENGINES_ODE_SIMULATOR_PLAYERODE_H
#define SIMULATOR_ENGINES_ODE_SIMULATOR_PLAYERODE_H

#include "simulator/player.h"
#include "xbee/shared/packettypes.h"
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

		bool chip_set, kick_set;

		double chip_strength, kick_strength;

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

		/**
		 * Target wheel velocities in quarter of a degree per 5 milliseconds.
		 */
		double motor_desired[4];

		/**
		 * Should the robot run in direct drive mode?
		 */
		bool direct_drive;

		/**
		 * Should the robot run in controlled drive mode?
		 */
		bool controlled_drive;

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
		bool robot_contains_shape_ground(dGeomID geom);

	protected:
		/**
		 * Controls the robot's movement.
		 *
		 * \param[in] vel desired robot velocity for control.
		 *
		 * \param[in] avel desired robot angular velocity.
		 */
		void move_impl(const Point &vel, double avel);

	public:
		bool hasContactPenetration(dVector3 pos);

		bool hasContactWithFace(dVector3 pos);
		/**
		 * Called when we find a robot-ball collision.
		 * May do some additional testing beyond this to make sure "has ball".
		 */
		void set_has_ball();

		void pre_tic(double TimeStep);

		void dribble(double speed);

		void kick(double strength);

		bool has_kick_set() {
			return kick_set;
		}

	private:
		bool execute_kick();

	public:
		void chip(double strength);

		bool has_chip_set() {
			return chip_set;
		}

	private:
		bool execute_chip();

	public:
		void position(const Point &pos);

		void velocity(const Point &vel);

		void orientation(double orient);

		void avelocity(double avel);

		void received(const XBeePacketTypes::RUN_DATA &);

		dTriMeshDataID create_robot_geom();
};

#endif

