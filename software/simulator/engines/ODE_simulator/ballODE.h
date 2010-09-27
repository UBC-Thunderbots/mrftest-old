#ifndef SIMULATOR_ENGINES_ODE_SIMULATOR_BALLODE_H
#define SIMULATOR_ENGINES_ODE_SIMULATOR_BALLODE_H

#include "simulator/ball.h"
#include <ode/ode.h>

/**
 * The back-end behind an ODE SimulatorBall object.
 */
class BallODE : public SimulatorBall {
	public:
		typedef RefPtr<BallODE> Ptr;
		dWorldID world;
		dBodyID body;
		dGeomID ballGeom;
		dMass m;


		BallODE(dWorldID dworld, dSpaceID dspace, double radius = 0.042672 / 2, double mass = 0.046);

		/*BallODE(dWorldID dworld, dSpaceID dspace, double radius);

		   BallODE(dWorldID dworld, dSpaceID dspace);
		 */
		~BallODE();

		Point position() const;

		Point velocity() const;

		Point acceleration() const;
		double get_height() const;

		double getRadius();

		void position(const Point &pos);
		void velocity(const Point &vel);

		bool in_goal();

	private:
		Point the_position, the_velocity;
		double dradius;
};

#endif

