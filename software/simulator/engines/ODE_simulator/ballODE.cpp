#include "simulator/engines/ODE_simulator/ballODE.h"
#include "simulator/field.h"
#include "util/codec.h"
#include "util/exception.h"
#include <cerrno>
#include <iostream>
#include <stdint.h>
#include <unistd.h>

#warning this class needs Doxygen comments in its header file
/*

   ballODE.h has the following:

   public:
   typedef RefPtr<BallODE> Ptr;
   dWorldID world;


   private:
   Point the_position, the_velocity;


 */


BallODE::BallODE(dWorldID dworld, dSpaceID dspace, dReal radius, dReal mass) : the_position(0.0, 0.0), the_velocity(0.0, 0.0) {
	world = dworld;


	body = dBodyCreate(world);
	ballGeom = dCreateSphere(dspace, radius); // golf ball radius 4.2672cm
	dGeomSetBody(ballGeom, body);
	dBodySetPosition(body, 0.0, 0.0, radius + static_cast<dReal>(0.001));


	dMassSetSphereTotal(&m, mass, radius);
	dBodySetMass(body, &m);

	// dSpaceAdd (dspace, ballGeom);
	dBodySetLinearDamping(body, static_cast<dReal>(0.001));

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

	dBodySetPosition(body, static_cast<dReal>(pos.x), static_cast<dReal>(pos.y), t[2]);
}

void BallODE::velocity(const Point &vel) {
	dBodySetLinearVel(body, static_cast<dReal>(vel.x), static_cast<dReal>(vel.y), 0.0);
	dBodySetAngularVel(body, 0.0, 0.0, 0.0);
}

bool BallODE::in_goal() {
	Point p;
	const dReal *t = dBodyGetPosition(body);
	p.x = t[0];
	p.y = t[1];
	double len = Simulator::Field::LENGTH;
	if (p.x > len / 2 || p.x < -len / 2) {
		double width = Simulator::Field::GOAL_WIDTH;

		if (p.y < width / 2 && p.y > (-width / 2)) {
			// double height = t[2];
			// if(height<fld->goal_height())return true;
			return true;
		}
	}


	return false;
}

void BallODE::load_state(FileDescriptor::Ptr fd) {
	double values[3 + 3 + 3];
	uint8_t buffer[sizeof(values) / sizeof(*values) * 8];
	ssize_t rc = read(fd->fd(), buffer, sizeof(buffer));
	if (rc < 0) {
		throw SystemError("read", errno);
	} else if (rc != sizeof(buffer)) {
		throw std::runtime_error("Premature EOF in state file");
	}
	for (std::size_t i = 0; i < sizeof(values) / sizeof(*values); ++i) {
		values[i] = decode_double(&buffer[i * 8]);
	}
	dBodySetPosition(body, static_cast<dReal>(values[0]), static_cast<dReal>(values[1]), static_cast<dReal>(values[2]));
	dBodySetLinearVel(body, static_cast<dReal>(values[3]), static_cast<dReal>(values[4]), static_cast<dReal>(values[5]));
	dBodySetAngularVel(body, static_cast<dReal>(values[6]), static_cast<dReal>(values[7]), static_cast<dReal>(values[8]));
}

void BallODE::save_state(FileDescriptor::Ptr fd) const {
	double values[3 + 3 + 3];
	const dReal *reals = dBodyGetPosition(body);
	values[0] = reals[0];
	values[1] = reals[1];
	values[2] = reals[2];
	reals = dBodyGetLinearVel(body);
	values[3] = reals[0];
	values[4] = reals[1];
	values[5] = reals[2];
	reals = dBodyGetAngularVel(body);
	values[6] = reals[0];
	values[7] = reals[1];
	values[8] = reals[2];

	uint8_t buffer[sizeof(values) / sizeof(*values) * 8];
	for (std::size_t i = 0; i < sizeof(values) / sizeof(*values); ++i) {
		encode_double(&buffer[i * 8], values[i]);
	}

	if (write(fd->fd(), buffer, sizeof(buffer)) != sizeof(buffer)) {
		throw SystemError("write", errno);
	}
}

