#include "simulator/engines/ballODE.h"

/*

ballODE.h has the following:

public:
typedef Glib::RefPtr<ballODE> ptr;
dWorldID world;


private:
point the_position, the_velocity;


*/

			
			ballODE::ballODE(dWorldID dworld, dSpaceID dspace) : the_position(0.0, 0.0), the_velocity(0.0, 0.0) {

			world = dworld;
			dMass m;

			dBodyID body = dBodyCreate(world);
			dBodySetPosition(body, 0.0, 0.0, 0.03);
			dGeomID ballGeom = dCreateSphere(0, 0.04267);//golf ball radius 4.2672cm

			dMassSetSphere (&m,1,0.04267);
			dBodySetMass (body,&m);

			dGeomSetBody (ballGeom,body);
			dSpaceAdd (dspace, ballGeom);


			}

			ballODE::~ballODE(){



			}

			point ballODE::position() const {
				return the_position;
			}

			point ballODE::velocity() const {
				return the_velocity;
			}

			point ballODE::acceleration() const {

					return point(0.0, 0.0);
			}

			void ballODE::ui_set_position(const point &pos) {
			}


