#include "simulator/engines/ballODE.h"
#include <iostream>
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

			body = dBodyCreate(world);
			dBodySetPosition(body, 0.0, 0.0, 0.15267);
			ballGeom = dCreateSphere(0, 0.14267);//golf ball radius 4.2672cm

			dMassSetSphere (&m,0.1,0.14267);
			dBodySetMass (body,&m);

			dGeomSetBody (ballGeom,body);
			dSpaceAdd (dspace, ballGeom);


			}

			ballODE::~ballODE(){



			}

			point ballODE::position() const {
point p;
const dReal *t = dBodyGetPosition (body);
p.x = t[0];
p.y = t[1];
//std::cout<<"ball"<<t[2]<<std::endl;
				return p;
			}

			point ballODE::velocity() const {
				return the_velocity;
			}

			point ballODE::acceleration() const {

					return point(0.0, 0.0);
			}

			void ballODE::ui_set_position(const point &pos) {
			}


