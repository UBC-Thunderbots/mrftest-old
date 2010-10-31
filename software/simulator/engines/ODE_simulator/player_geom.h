#ifndef SIMULATOR_ENGINES_ODE_SIMULATOR_PLAYER_GEOM_H
#define SIMULATOR_ENGINES_ODE_SIMULATOR_PLAYER_GEOM_H


#include <ode/ode.h>
#include "util/noncopyable.h"

class Player_geom : public NonCopyable {
	public:
		Player_geom(dWorldID eworld, dSpaceID dspace) : world(eworld), space(dspace) {
			body = dBodyCreate(world);
		}

		~Player_geom() {
			dBodyDestroy(body);
		}

		virtual void handle_collision(dGeomID o1, dGeomID o2, dJointGroupID contactgroup) = 0;
		virtual void reset_frame() {
		}
		virtual bool has_ball() const = 0;
		
		virtual bool has_geom(dGeomID geom){
			return body == dGeomGetBody(geom);
		}
		/**
		 * The ID for the robot's body in the simulator.
		 */
		dBodyID body;

	protected:
		dWorldID world;
		dSpaceID space;
};


#endif

