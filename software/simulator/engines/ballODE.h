#include "world/ball_impl.h"
#include <ode/ode.h>

//
// The back-end behind an ODE ball object.
// 
//
class ballODE : public ball_impl {
	public:

			typedef Glib::RefPtr<ballODE> ptr;
			dWorldID world;
			dBodyID body;
			dGeomID ballGeom;
			
			
			ballODE(dWorldID dworld, dSpaceID dspace, double radius, double mass);
			
			ballODE(dWorldID dworld, dSpaceID dspace, double radius);
			
			ballODE(dWorldID dworld, dSpaceID dspace);
			
			~ballODE();

			point position() const;

			point velocity() const;

			point acceleration() const;

			void ui_set_position(const point &pos);

	private:
			point the_position, the_velocity;

};


