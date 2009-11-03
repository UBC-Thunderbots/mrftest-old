#include "world/ball_impl.h"
#include <ode/ode.h>

//
// The back-end behind an ODE player object.
// 
//
class ballODE : public virtual ball_impl {
	public:
//The world constructed by the simulatiuon engine
 

typedef Glib::RefPtr<ballODE> ptr;
	dWorldID world;
	ballODE(dWorldID dworld);


			virtual point position() const;

			virtual point velocity() const;

			virtual point acceleration() const;

			virtual void ui_set_position(const point &pos);

	private:
			point the_position, the_velocity;

};


