#ifndef SIMULATOR_ENGINES_ODE_SIMULATOR_BALLODE_H
#define SIMULATOR_ENGINES_ODE_SIMULATOR_BALLODE_H

#include "simulator/ball.h"
#include "util/fd.h"
#include <ode/ode.h>

/**
 * The back-end behind an ODE SimulatorBall object.
 */
class BallODE : public Simulator::Ball {
	public:
		typedef RefPtr<BallODE> Ptr;
		dWorldID world;
		dBodyID body;
		dGeomID ballGeom;
		dMass m;


		BallODE(dWorldID dworld, dSpaceID dspace, dReal radius = static_cast<dReal>(0.042672) / 2, dReal mass = static_cast<dReal>(0.046));

		/*BallODE(dWorldID dworld, dSpaceID dspace, double radius);

		   BallODE(dWorldID dworld, dSpaceID dspace);
		 */
		Point position() const;

		Point velocity() const;

		Point acceleration() const;
		double get_height() const;

		double getRadius();

		void position(const Point &pos);
		void velocity(const Point &vel);

		bool in_goal();

		void load_state(FileDescriptor::Ptr fd);
		void save_state(FileDescriptor::Ptr fd) const;

	private:
		Point the_position, the_velocity;
		double dradius;
};

#endif

