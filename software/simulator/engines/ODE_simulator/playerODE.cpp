#include "simulator/engines/ODE_simulator/playerODE.h"
#include "geom/angle.h"
#include "simulator/engines/ODE_simulator/compo_player_geom.h"
#include "util/timestep.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>

#warning this class needs Doxygen comments in its header file

namespace {
	dReal orientationFromMatrix(const dReal *t) {
		return std::atan2(-t[1], t[0]);
	}

	bool isTipped(const dBodyID body) {
		const dReal *t = dBodyGetRotation(body);
		return std::fabs(t[8] - 1) < 0.1;
	}

	void rotate_vec(dVector3 &vec, const dReal *r) {
		dVector3 ans;
		ans[0] = r[0] * vec[0] + r[1] * vec[1] + r[2] * vec[2];
		ans[1] = r[4] * vec[0] + r[5] * vec[1] + r[6] * vec[2];
		ans[2] = r[8] * vec[0] + r[8] * vec[1] + r[10] * vec[2];

		vec[0] = ans[0];
		ans[1] = ans[1];
		ans[2] = ans[2];
	}

	/**
	 * Conversion Factor from the value used in radio packets (1/4 degree) per 5 ms to motor voltage
	 */
	const dReal PACKET_TO_VOLTAGE = static_cast<dReal>(0.022281639);


	/**
	 * Maximum voltage available to the robot.
	 */
	const dReal VOLTAGE_LIMIT = 15.0;


	/**
	 * Resistance of the stator
	 */
	const dReal MOTOR_RESISTANCE = static_cast<dReal>(1.2); // ohms

	/**
	 * Constant relating motor current to motor torque
	 */
	const dReal CURRENT_TO_TORQUE = static_cast<dReal>(0.0255); // Nm / amp

	/**
	 * Gearing ration of the drive train (speed reduction, torque increase)
	 */
	const dReal GEAR_RATIO = 3.5;

	/**
	 * Radius of the wheel for torque to force calculations
	 */
	const dReal WHEEL_RADIUS = static_cast<dReal>(0.0254);

	/**
	 * physical radius of the robot in meters
	 */
	const dReal ROBOT_RADIUS = static_cast<dReal>(0.09);

	/**
	 * Physical mass of the robot in kg (measured)
	 */
	const dReal ROBOT_MASS = static_cast<dReal>(2.552);

	/**
	 * Physical height of the robot in m
	 */
	const dReal ROBOT_HEIGHT = static_cast<dReal>(0.15);

	/**
	 * Physical width of the flattened face of the robot
	 */
	const dReal FRONT_FACE_WIDTH = static_cast<dReal>(0.16);


	/**
	 * Angles in radians that the wheels are located off the forward direction
	 */
	const dReal ANGLES[4] = { static_cast<dReal>(0.959931), static_cast<dReal>(2.35619), static_cast<dReal>(3.9269908), static_cast<dReal>(5.32325) };

	int click = 0;
}

/*
    Constructor method for the robot model contained in the simulator
 */
PlayerODE::PlayerODE(dWorldID eworld, dSpaceID dspace, dGeomID ballGeomi, dReal ups_per_tick) : p_geom(eworld, dspace) {
	orders.kick = false;
	orders.chip = false;
	std::fill(&orders.wheel_speeds[0], &orders.wheel_speeds[4], 0);

	updates_per_tick = ups_per_tick;
	ballGeom = ballGeomi;
	world = eworld;
	body = p_geom.body;
	// to account for a lower centre of mass make the cylinder much lower
	// when calculating the mass object
	dMassSetCylinderTotal(&mass, ROBOT_MASS, 3, ROBOT_RADIUS, ROBOT_RADIUS);
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
	delete[] wheel_position;
	delete[] force_direction;
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

void PlayerODE::dribble(dReal set_point) {
	dVector3 vec = { 0.0, -1.0, 0.0 };
	rotate_vec(vec, dBodyGetRotation(body));
	// dGeomGetBody(ballGeom)
	// 3.30 mNm/A
	// starting amps = 9.47
	// 31.251
	// say we want ball to spin at 200 rmp
	// 0.1047 convert rpm to rad/sec

	dReal mx_torque = static_cast<dReal>(0.001 * 31.251);

	// speed of spinning ball
	dReal rpm = 100;
	dReal speed = rpm * static_cast<dReal>(0.1047);

	dReal x = dBodyGetAngularVel(dGeomGetBody(ballGeom))[0] * vec[0];
	dReal y = dBodyGetAngularVel(dGeomGetBody(ballGeom))[1] * vec[1];
	dReal z = dBodyGetAngularVel(dGeomGetBody(ballGeom))[2] * vec[2];

	dReal act_speed = x + y + z;

	act_speed = std::min(speed, act_speed);
	act_speed = std::max(static_cast<dReal>(0.0), act_speed);

	dReal torque = ((speed - act_speed) / speed) * mx_torque;
	dBodyAddTorque(dGeomGetBody(ballGeom), torque * vec[0], torque * vec[1], torque * vec[2]);
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
dReal PlayerODE::get_height() const {
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
void PlayerODE::pre_tic(dReal) {
	// limit max motor "voltage" to VOLTAGE_LIMIT by scaling the largest component to VOLTAGE_LIMIT if greater but preserve its orientation

	dReal max_speed = 0.0;
	for (std::size_t index = 0; index < 4; index++) {
		max_speed = std::max(max_speed, std::fabs(static_cast<dReal>(orders.wheel_speeds[index])));
	}
	if (max_speed > VOLTAGE_LIMIT / PACKET_TO_VOLTAGE) {
		for (std::size_t index = 0; index < 4; index++) {
			orders.wheel_speeds[index] *= static_cast<int>(VOLTAGE_LIMIT / (PACKET_TO_VOLTAGE * max_speed));
		}
	}

	click++;

	Point force;

	if (isTipped(body)) {
		// if the robot is tipped then put it upright
		std::cout << "tipped " << std::endl;
		const dReal *rot = dBodyGetRotation(body);
		dVector3 vec = { 1, 1, 0 };
		rotate_vec(vec, rot);
		dReal angle = std::atan2(vec[0], vec[1]);
		dReal c = std::cos(angle);
		dReal s = std::sin(angle);
		dMatrix3 rota = { c, -s, 0.0, 0.0, s, c, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0 };
		dBodySetRotation(body, rota);
	}

	if (!posSet && !isTipped(body)) {
		// get the current bots velocity
		const dReal *cur_vel = dBodyGetLinearVel(body);
		// grab current vel
		const Point the_velocity = Point(cur_vel[0], cur_vel[1]);
		// get the angular velocity
		const dReal *avels = dBodyGetAngularVel(body);
		Point want_vel;
		dReal want_ang;

		// This is the pseudo-inverse of the matrix used to go from wanted velocities to motor set points
		want_vel.x = -0.0068604 * orders.wheel_speeds[0] + -0.0057842 * orders.wheel_speeds[1] + 0.0057842 * orders.wheel_speeds[2] + 0.0068604 * orders.wheel_speeds[3];
		want_vel.y = 0.0078639 * orders.wheel_speeds[0] + -0.0078639 * orders.wheel_speeds[1] +  -0.0078639 * orders.wheel_speeds[2] + 0.0078639 * orders.wheel_speeds[3];
		want_ang = static_cast<dReal>(0.0654194 * (orders.wheel_speeds[0] + orders.wheel_speeds[1] + orders.wheel_speeds[2] + orders.wheel_speeds[3]));

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
		dReal torque = (want_ang - avels[2]) / static_cast<dReal>(5.0) * static_cast<dReal>(12.0) * momentInertia;

		dBodyEnable(body);
		dBodySetDynamic(body);

		dBodyAddTorque(body, 0.0, 0.0, 2 * torque);
		dBodyAddForce(body, static_cast<dReal>(fce.x), static_cast<dReal>(fce.y), 0.0);

		if (has_ball()) {
			dribble(static_cast<dReal>(0.1));
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
	dReal strength = static_cast<dReal>(orders.chick_power);
	dReal maximum_impulse = 1.0;
	Point direction(1.0, 0.0);
	direction = direction.rotate(orientation());
	Point impulse = strength * maximum_impulse * direction;
	dReal zimpulse = strength * maximum_impulse / static_cast<dReal>(std::sqrt(2.0));

	if (has_ball()) {
		dVector3 force;
		dWorldImpulseToForce(world, static_cast<dReal>(1.0) / (static_cast<dReal>(TIMESTEPS_PER_SECOND) * updates_per_tick),
		                     static_cast<dReal>(impulse.x), static_cast<dReal>(impulse.y), zimpulse, force);
		dBodyAddForce(dGeomGetBody(ballGeom), force[0], force[1], force[2]);
	}
	return has_ball();
}

bool PlayerODE::execute_chip() {
	dReal strength = static_cast<dReal>(orders.chick_power);
	dReal maximum_impulse = 1.0;

	Point direction(1.0 / std::sqrt(2.0), 0.0);
	direction = direction.rotate(orientation());
	Point impulse = strength * maximum_impulse * direction;
	dReal zimpulse = strength * maximum_impulse / static_cast<dReal>(std::sqrt(2.0));

	if (has_ball()) {
		dVector3 force;

		dWorldImpulseToForce(world, static_cast<dReal>(1.0) / (static_cast<dReal>(TIMESTEPS_PER_SECOND) * updates_per_tick),
		                     static_cast<dReal>(impulse.x), static_cast<dReal>(impulse.y), zimpulse, force);
		dBodyAddForce(dGeomGetBody(ballGeom), force[0], force[1], force[2]);
	}

	return has_ball();
}

void PlayerODE::position(const Point &pos) {
	posSet = true;
	dBodySetPosition(body, static_cast<dReal>(pos.x), static_cast<dReal>(pos.y), static_cast<dReal>(ROBOT_HEIGHT) / 2 + static_cast<dReal>(0.01));
}

void PlayerODE::velocity(const Point &vel) {
	dBodySetLinearVel(body, static_cast<dReal>(vel.x), static_cast<dReal>(vel.y), 0.0);
}

void PlayerODE::orientation(double orient) {
	posSet = true;
	dMatrix3 RotationMatrix;
	dRFromAxisAndAngle(RotationMatrix, 0.0, 0.0, 1.0, static_cast<dReal>(orient));
	dBodySetRotation(body, RotationMatrix);
}

void PlayerODE::avelocity(double avel) {
	dBodySetAngularVel(body, 0.0, 0.0, static_cast<dReal>(avel));
}

