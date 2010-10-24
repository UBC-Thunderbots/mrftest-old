#ifndef SIMULATOR_ENGINES_ODE_SIMULATOR_COMPO_PLAYER_GEOM_H
#define SIMULATOR_ENGINES_ODE_SIMULATOR_COMPO_PLAYER_GEOM_H


#include "player_geom.h"
#include <ode/ode.h>

class Compo_player_geom : public Player_geom {
	public:
		
		Compo_player_geom(dWorldID eworld, dSpaceID dspace);
		~Compo_player_geom();
		void handle_collision(dGeomID o1, dGeomID o2, dJointGroupID contactgroup);
		void reset_frame();
		bool has_ball();
	private:
		
		bool robot_contains_shape(dGeomID geom);
		void handleRobotBallCollision(dGeomID o1, dGeomID o2, dJointGroupID contactgroup);
		void handleCollisionWithGround(dGeomID o1, dGeomID o2, dJointGroupID contactgroup);
		
		void handleRobotRobotCollision(dGeomID o1, dGeomID o2, dJointGroupID contactgroup);
		void handleWallCollision(dGeomID o1, dGeomID o2, dJointGroupID contactgroup);
		//bool hasContactPenetration(dVector3 pos, dGeomID geom);
		
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


};


#endif
