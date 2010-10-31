#include "simulator/engines/ODE_simulator/playerODE.h"
#include "geom/angle.h"
#include "util/timestep.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include "compo_player_geom.h"

#warning this class needs Doxygen comments in its header file

namespace {
	double orientationFromMatrix(const dReal *t) {
		return atan2(-t[1], t[0]);
	}

	bool isTipped(const dBodyID body) {
		const dReal *t = dBodyGetRotation(body);
		return fabs(t[8] - 1) < 0.1;
	}

	/**
	 * Conversion Factor from the value used in radio packets (1/4 degree) per 5 ms to motor voltage
	 */
	const double PACKET_TO_VOLTAGE = 0.022281639;


	/**
	 * Maximum voltage available to the robot.
	 */
	const double VOLTAGE_LIMIT = 15.0;


	/**
	 * Resistance of the stator
	 */
	const double MOTOR_RESISTANCE = 1.2; // ohms

	/**
	 * Constant relating motor current to motor torque
	 */
	const double CURRENT_TO_TORQUE = 0.0255; // Nm / amp

	/**
	 * Gearing ration of the drive train (speed reduction, torque increase)
	 */
	const double GEAR_RATIO = 3.5;

	/**
	 * Radius of the wheel for torque to force calculations
	 */
	const double WHEEL_RADIUS = 0.0254;

	/**
	 * physical radius of the robot in meters
	 */
	const double ROBOT_RADIUS = 0.09;

	/**
	 * Physical mass of the robot in kg (measured)
	 */
	const double ROBOT_MASS = 2.552;

	/**
	 * Physical height of the robot in m
	 */
	const double ROBOT_HEIGHT = 0.15;

	/**
	 * Physical width of the flattened face of the robot
	 */
	const double FRONT_FACE_WIDTH = 0.16;


	/**
	 * Angles in radians that the wheels are located off the forward direction
	 */
	const double ANGLES[4] = { 0.959931, 2.35619, 3.9269908, 5.32325 };

	int click = 0;
}

/*
    Constructor method for the robot model contained in the simulator
 */
PlayerODE::PlayerODE(dWorldID eworld, dSpaceID dspace, dGeomID ballGeomi, double ups_per_tick) : p_geom(eworld, dspace) {
	orders.kick = false;
	orders.chip = false;
	std::fill(&orders.wheel_speeds[0], &orders.wheel_speeds[4], 0);

	updates_per_tick = ups_per_tick;
	double dribble_radius = 0.005; // half a cm
	ballGeom = ballGeomi;
	double ballradius = dGeomSphereGetRadius(ballGeom);
	world = eworld;
	body = p_geom.body;
	dMassSetCylinderTotal(&mass, ROBOT_MASS, 3, ROBOT_RADIUS, ROBOT_HEIGHT);
	dBodySetMass(body, &mass);
	momentInertia = ROBOT_RADIUS * ROBOT_RADIUS * mass.mass / 2;

	wheel_position = new Point[4];
	force_direction = new Point[4];

	for (int index = 0; index < 4; index++) {
		wheel_position[index] = Point(1, 0).rotate(ANGLES[index]) * ROBOT_RADIUS;
		force_direction[index] = Point(wheel_position[index].rotate(M_PI / 2).norm());
	}
}

PlayerODE::~PlayerODE() {
	if (wheel_position != NULL) {
		delete[] wheel_position;
	}

	if (force_direction != NULL) {
		delete[] force_direction;
	}
}

// Accessor method to get the robots position
Point PlayerODE::position() const {
	const dReal *t = dBodyGetPosition(body);
	return Point(t[0], t[1]);
}

// Accessor method to get the robots orientation
double PlayerODE::orientation() const {
	return orientationFromMatrix(dBodyGetRotation(body));
}

/*
   Returns whether or not a given robot has the ball.
   Has ball is determined from the collision detection from the previous timestep
 */
bool PlayerODE::has_ball() const {
	return p_geom.has_ball();
}

unsigned int PlayerODE::dribbler_speed() const {
	return has_ball() ? 30 : 50;
}

// Accessor to get the height of the middle of the robot (should be ROBOT_HEIGHT/2)
double PlayerODE::get_height() const {
	const dReal *t = dBodyGetPosition(body);
	return t[2];
}


bool PlayerODE::robot_contains_shape(dGeomID geom) {
	return p_geom.has_geom(geom);
}

/*
   computes the forces for the differential equation and adds them to the robot body
   \param[in] timestep the time between calculations
 */
void PlayerODE::pre_tic(double) {
	// limit max motor "voltage" to VOLTAGE_LIMIT by scaling the largest component to VOLTAGE_LIMIT if greater but preserve its orientation

	double max_speed = 0.0;
	for (uint8_t index = 0; index < 4; index++) {
		if (fabs(orders.wheel_speeds[index]) > max_speed) {
			max_speed = fabs(orders.wheel_speeds[index]);
		}
	}
	if (max_speed > VOLTAGE_LIMIT / PACKET_TO_VOLTAGE) {
		for (int8_t index = 0; index < 4; index++) {
			orders.wheel_speeds[index] *= VOLTAGE_LIMIT / (PACKET_TO_VOLTAGE * max_speed);
		}
	}

	click++;

	// Current Motor Speed
	double motor_current[4];
	double wheel_torque;
	Point force;

	if (!posSet) {
		// get the current bots velocity
		const dReal *cur_vel = dBodyGetLinearVel(body);
		// grab current vel
		const Point the_velocity = Point(cur_vel[0], cur_vel[1]);
		// get the angular velocity
		const dReal *avels = dBodyGetAngularVel(body);
		Point want_vel;
		double want_ang;

		// This is the pseudo-inverse of the matrix used to go from wanted velocities to motor set points
		want_vel.x = -0.0068604 * orders.wheel_speeds[0] + - 0.0057842 * orders.wheel_speeds[1] + 0.0057842 * orders.wheel_speeds[2] + 0.0068604 * orders.wheel_speeds[3];
		want_vel.y = 0.0078639 * orders.wheel_speeds[0] + - 0.0078639 * orders.wheel_speeds[1] +  - 0.0078639 * orders.wheel_speeds[2] + 0.0078639 * orders.wheel_speeds[3];
		want_ang = 0.0654194 * (orders.wheel_speeds[0] + orders.wheel_speeds[1] + orders.wheel_speeds[2] + orders.wheel_speeds[3]);

		if (want_vel.len() > 5.0) {
			want_vel = 5.0 * want_vel / want_vel.len();
		}
		if (want_ang > 5.0) {
			want_ang = 5.0;
		}
		if (want_ang < -5.0) {
			want_ang = -5.0;
		}

		// change robot relative velocity to absolute
		want_vel = want_vel.rotate(orientation());
		Point fce = (want_vel - the_velocity) / 5.0 * 10.0 * mass.mass;
		double torque = (want_ang - avels[2]) / 5.0 * 12.0 * momentInertia;

		dBodyEnable(body);
		dBodySetDynamic(body);

		if (isTipped(body)) {
			std::cout << "tipped " << std::endl;
		} else {
			dBodyAddTorque(body, 0.0, 0.0, 2 * torque);
			dBodyAddForce(body, fce.x, fce.y, 0.0);
		}

		if (has_chip_set() && has_ball()) {
			if (execute_chip()) {
				orders.chip = false;
			}
		} else if (has_kick_set() && has_ball()) {
			if (execute_kick()) {
				orders.kick = false;
			}
		}
	}
	p_geom.reset_frame();
	posSet = false;
}

bool PlayerODE::execute_kick() {
	double strength = orders.chick_power;
	double maximum_impulse = 1.0;
	Point direction(1.0, 0.0);
	direction = direction.rotate(orientation());
	Point impulse = strength * maximum_impulse * direction;
	double zimpulse = strength * maximum_impulse / sqrt(2.0);

	if (has_ball()) {
		dVector3 force;
		dWorldImpulseToForce(world, 1.0 / (static_cast<double>(TIMESTEPS_PER_SECOND) * updates_per_tick),
		                     impulse.x, impulse.y, zimpulse, force);
		dBodyAddForce(dGeomGetBody(ballGeom), force[0], force[1], force[2]);
	}
	return has_ball();
}

bool PlayerODE::execute_chip() {
	double strength = orders.chick_power;
	double maximum_impulse = 1.0;

	Point direction(1.0 / sqrt(2.0), 0.0);
	direction = direction.rotate(orientation());
	Point impulse = strength * maximum_impulse * direction;
	double zimpulse = strength * maximum_impulse / sqrt(2.0);

	if (has_ball()) {
		dVector3 force;

		dWorldImpulseToForce(world, 1.0 / (static_cast<double>(TIMESTEPS_PER_SECOND) * updates_per_tick),
		                     impulse.x, impulse.y, zimpulse, force);
		dBodyAddForce(dGeomGetBody(ballGeom), force[0], force[1], force[2]);
	}

	return has_ball();
}

void PlayerODE::position(const Point &pos) {
	posSet = true;
	dBodySetPosition(body, pos.x, pos.y, ROBOT_HEIGHT / 2 + 0.01);
}

void PlayerODE::velocity(const Point &vel) {
	dBodySetLinearVel(body, vel.x, vel.y, 0.0);
}

void PlayerODE::orientation(double orient) {
	posSet = true;
	dMatrix3 RotationMatrix;
	dRFromAxisAndAngle(RotationMatrix, 0.0, 0.0, 1.0, orient);
	dBodySetRotation(body, RotationMatrix);
}

void PlayerODE::avelocity(double avel) {
	dBodySetAngularVel(body, 0.0, 0.0, avel);
}

