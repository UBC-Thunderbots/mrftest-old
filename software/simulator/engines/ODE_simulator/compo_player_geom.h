#ifndef SIMULATOR_ENGINES_ODE_SIMULATOR_COMPO_PLAYER_GEOM_H
#define SIMULATOR_ENGINES_ODE_SIMULATOR_COMPO_PLAYER_GEOM_H

#include "player_geom.h"
#include <ode/ode.h>

class CompoPlayerGeom : public PlayerGeom {
	public:
		CompoPlayerGeom(dWorldID eworld, dSpaceID dspace);
		~CompoPlayerGeom();
		void handle_collision(dGeomID o1, dGeomID o2, dJointGroupID contactgroup);
		void reset_frame();
		bool has_ball() const;
		bool has_geom(dGeomID geom);

	private:
		void handleRobotBallCollision(dGeomID o1, dGeomID o2, dJointGroupID contactgroup);
		void handleCollisionWithGround(dGeomID o1, dGeomID o2, dJointGroupID contactgroup);

		void handleRobotRobotCollision(dGeomID o1, dGeomID o2, dJointGroupID contactgroup);
		void handleWallCollision(dGeomID o1, dGeomID o2, dJointGroupID contactgroup);
		// bool hasContactPenetration(dVector3 pos, dGeomID geom);
		void dribble(double set_point);

		/**
		 * The rectangular geometry for the front collision.
		 */
		dGeomID robotGeomTop;

		/**
		 * The Cylindrical geometry for most collisions.
		 */
		dGeomID robotGeomTopCyl;

		dGeomID dribblerBar;

		// dBodyID dribbler;

		/**
		 * Used to determine whether the player has ball
		 */
		bool has_ball_now;
};


#endif

