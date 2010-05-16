#include "sim/ball.h"
#include <ode/ode.h>

//
// The back-end behind an ODE ball object.
// 
//
class ballODE : public ball {
	public:

			typedef Glib::RefPtr<ballODE> ptr;
			dWorldID world;
			dBodyID body;
			dGeomID ballGeom;
			dMass m;
			
			
			ballODE(dWorldID dworld, dSpaceID dspace, double radius   = 0.042672/2, double mass = 0.046);
			
			/*ballODE(dWorldID dworld, dSpaceID dspace, double radius);
			
			ballODE(dWorldID dworld, dSpaceID dspace);
			*/
			~ballODE();

			point position() const;

			point velocity() const;

			point acceleration() const;
			double get_height() const;

			double getRadius();

			void position(const point &pos);
			void velocity(const point &vel);

			bool in_goal();

	private:
			point the_position, the_velocity;
			 double dradius;
			

};


