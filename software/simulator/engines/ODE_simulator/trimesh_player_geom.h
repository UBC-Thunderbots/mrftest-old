#ifndef SIMULATOR_ENGINES_ODE_SIMULATOR_TRIMESH_PLAYER_GEOM_H
#define SIMULATOR_ENGINES_ODE_SIMULATOR_TRIMESH_PLAYER_GEOM_H


#include "player_geom.h"
#include <ode/ode.h>

class Trimesh_player_geom : public Player_geom {
	public:
		
		Trimesh_player_geom(dWorldID eworld, dSpaceID dspace);
		~Trimesh_player_geom();
		void handle_collision(dGeomID o1, dGeomID o2, dJointGroupID contactgroup);
		void reset_frame();
		bool has_ball() const;
	private:
		bool hasContactWithFace(dVector3 pos, dGeomID geom);
		bool robot_contains_shape(dGeomID geom);

		
		/**
		 * The robot geometry
		 */
		dTriMeshDataID robotGeom;

		dTriMeshDataID create_robot_geom();
		
		/**
		 * Used to determine whether the player has ball
		 */
		bool has_ball_now;
		
		dVector3 *Vertices;
		unsigned int *Triangles;


};


#endif
