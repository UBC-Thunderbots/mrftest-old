#include "simulator/engines/ODE_simulator/ballODE.h"
#include "simulator/field.h"
#include <iostream>

#warning this class needs Doxygen comments in its header file
/*

   ballODE.h has the following:

   public:
   typedef RefPtr<BallODE> Ptr;
   dWorldID world;


   private:
   Point the_position, the_velocity;


 */


BallODE::BallODE(dWorldID dworld, dSpaceID dspace, double radius, double mass) : the_position(0.0, 0.0), the_velocity(0.0, 0.0) {
	world = dworld;


	body = dBodyCreate(world);
	ballGeom = dCreateSphere(dspace, radius); // golf ball radius 4.2672cm
	dGeomSetBody(ballGeom, body);
	dBodySetPosition(body, 0.0, 0.0, radius + 0.001);


	dMassSetSphereTotal(&m, mass, radius);
	dBodySetMass(body, &m);

	// dSpaceAdd (dspace, ballGeom);
	dBodySetLinearDamping(body, 0.001);

	// dBodySetMaxAngularSpeed (body, 5.0);
}
double BallODE::get_height() const {
	const dReal *t = dBodyGetPosition(body);
	return t[2];
}

/*


    BallODE::BallODE(dWorldID dworld, dSpaceID dspace, double radius): the_position(0.0, 0.0), the_velocity(0.0, 0.0){

        world = dworld;
        dMass m;

        body = dBodyCreate(world);
        dBodySetPosition(body, 0.0, 0.0, radius +0.01);
        ballGeom = dCreateSphere(0, radius);//golf ball radius 4.2672cm

        dMassSetSphere (&m,2.0,radius);
        dBodySetMass (body,&m);

        dGeomSetBody (ballGeom,body);
        dSpaceAdd (dspace, ballGeom);
        dBodySetLinearDamping (body, 0.9);
         field::Ptr fldd(new SimulatorField);
         fld = fldd;

    }

    BallODE::BallODE(dWorldID dworld, dSpaceID dspace): the_position(0.0, 0.0), the_velocity(0.0, 0.0){

        world = dworld;
        dMass m;

        body = dBodyCreate(world);
        dBodySetPosition(body, 0.0, 0.0, 0.01086);

        dradius = 0.0213;

        ballGeom = dCreateSphere(0, dradius);//golf ball radius 4.2672cm

        dMassSetSphere (&m,1.0,0.0213);
        dBodySetMass (body,&m);

        dGeomSetBody (ballGeom,body);
        dSpaceAdd (dspace, ballGeom);
        //dBodySetLinearDamping (body, 0.2);
        dBodySetAngularDamping (body,0.5);
        dBodySetMaxAngularSpeed (body, 5.0);
         field::Ptr fldd(new SimulatorField);
         fld = fldd;

    }
 */
BallODE::~BallODE() {
}

double BallODE::getRadius() {
	return dradius;
}

Point BallODE::position() const {
	Point p;
	const dReal *t = dBodyGetPosition(body);
	p.x = t[0];
	p.y = t[1];
	// std::cout<<"ball"<<t[2]<<std::endl;
	return p;
}

Point BallODE::velocity() const {
	return the_velocity;
}

Point BallODE::acceleration() const {
	return Point(0.0, 0.0);
}

void BallODE::position(const Point &pos) {
	const dReal *t = dBodyGetPosition(body);

	dBodySetPosition(body, pos.x, pos.y, t[2]);
}

void BallODE::velocity(const Point &vel) {
	dBodySetLinearVel(body, vel.x, vel.y, 0.0);
	dBodySetAngularVel(body, 0.0, 0.0, 0.0);
}

bool BallODE::in_goal() {
	Point p;
	const dReal *t = dBodyGetPosition(body);
	p.x = t[0];
	p.y = t[1];
	double len = SimulatorField::LENGTH;
	if (p.x > len / 2 || p.x < -len / 2) {
		double width = SimulatorField::GOAL_WIDTH;

		if (p.y < width / 2 && p.y > (-width / 2)) {
			// double height = t[2];
			// if(height<fld->goal_height())return true;
			return true;
		}
	}


	return false;
}

