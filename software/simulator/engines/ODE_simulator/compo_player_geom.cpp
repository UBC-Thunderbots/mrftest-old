#include "compo_player_geom.h"
#include "geom/angle.h"
#include <cmath>
#include <iostream>

namespace {
	double orientationFromMatrix(const dReal *t) {
		return atan2(-t[1], t[0]);
	}
//
//
//
	const double CFM = 1E-5;

//
//
//
	const double ERP = 1.0;


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
	 * Radius of the dribbler
	 */
	const double DRIBBLER_RADIUS = 0.01468;

	/**
	 * Radius of the dribbler
	 */
	const double DRIBBLER_HEIGHT = 0.03783;

	const double MAX_VOLTAGE = 12.0;

	const double RPM_PER_VOLT = 2900;
}

Compo_player_geom::Compo_player_geom(dWorldID eworld, dSpaceID dspace) : Player_geom(eworld, dspace) {
	double dribble_radius = 0.005; // half a cm
	double ballradius = 0.042672 / 2;

	double x_pos = 0.0;
	double y_pos = 0.0;

	double y_len = 0.1;
	double x_len = sqrt((2 * ROBOT_RADIUS) * (2 * ROBOT_RADIUS) - (y_len) * (y_len));

	robotGeomTop = dCreateBox(0, x_len, y_len, ROBOT_HEIGHT);
	dGeomSetBody(robotGeomTop, body);
	robotGeomTopCyl = dCreateCapsule(0, ROBOT_RADIUS, ROBOT_HEIGHT);
	dGeomSetBody(robotGeomTopCyl, body);
	dBodySetPosition(body, x_pos, y_pos, ROBOT_HEIGHT / 2 + 0.001);

	// we need to rotate the
	// dribbler bar -90 degrees about the x-axis
	const dMatrix3 rotat = {
		1, 0, 0, 0,
		0, 0, 1, 1,
		0, 1, 0, 0
	};

	dribblerBar = dCreateCapsule(0, DRIBBLER_RADIUS, y_len);
	dMass mass;
	dMassSetCylinderTotal(&mass, 0.02, 3, DRIBBLER_RADIUS, y_len);
	dSpaceAdd(dspace, robotGeomTop);
	dSpaceAdd(dspace, robotGeomTopCyl);
	dSpaceAdd(dspace, dribblerBar);
	dGeomSetRotation(dribblerBar, rotat);
}

Compo_player_geom::~Compo_player_geom() {
	dGeomDestroy(robotGeomTop);
	dGeomDestroy(robotGeomTopCyl);
	dGeomDestroy(dribblerBar);
	dJointGroupDestroy(jGroup);
}

bool Compo_player_geom::has_geom(dGeomID geom) {
	dBodyID b = dGeomGetBody(geom);
	return b == body;
}


void Compo_player_geom::handle_collision(dGeomID o1, dGeomID o2, dJointGroupID contactgroup) {
	if (has_geom(o1) && has_geom(o2)) {
		return;
	}
	dGeomID robotGeom = o1;
	if (has_geom(o1)) {
		robotGeom = o2;
	}
	if (dGeomGetBody(o1) == 0 || dGeomGetBody(o2) == 0) {
		if (dGeomGetClass(o1) == dPlaneClass || dGeomGetClass(o2) == dPlaneClass) {
			handleCollisionWithGround(o1, o2, contactgroup);
		} else {
			handleWallCollision(o1, o2, contactgroup);
		}
	} else if (dGeomGetClass(o1) == dSphereClass || dGeomGetClass(o2) == dSphereClass) {
		handleRobotBallCollision(o1, o2, contactgroup);
	} else {
		handleRobotRobotCollision(o1, o2, contactgroup);
	}
}



bool hasContactPenetration(dVector3 pos, dGeomID geom) {
	if (dGeomGetClass(geom) == dBoxClass && dGeomBoxPointDepth(geom, pos[0], pos[1], pos[2]) < 0) {
		return true;
	}

	double y_len = 0.1;
	double x_len = sqrt((2 * ROBOT_RADIUS) * (2 * ROBOT_RADIUS) - (y_len) * (y_len));

	const dReal *p = dBodyGetPosition(dGeomGetBody(geom));

	Point ball_loc(pos[0], pos[1]);
	Point play_loc(p[0], p[1]);
	Point play_ball_diff = ball_loc - play_loc;
	play_ball_diff = play_ball_diff.rotate(-orientationFromMatrix(dBodyGetRotation(dGeomGetBody(geom))));

	bool face = play_ball_diff.x >= x_len / 2 && (play_ball_diff.y * x_len) <= (play_ball_diff.x * y_len);

	if (!face && pos[2] > 0 && pos[2] < ROBOT_HEIGHT && dGeomGetClass(geom) == dCCylinderClass) {
		return dGeomCapsulePointDepth(geom, pos[0], pos[1], pos[2]) < 0;
	}

	return false;
}



/*
   sees whether the contact joint penetrates the face of the robot
 */
bool hasContactWithFace(dVector3 pos, dGeomID geom) {
	double y_len = 0.1;
	double x_len = sqrt((2 * ROBOT_RADIUS) * (2 * ROBOT_RADIUS) - (y_len) * (y_len));
	const dReal *p = dBodyGetPosition(dGeomGetBody(geom));
	Point ball_loc(pos[0], pos[1]);
	Point play_loc(p[0], p[1]);
	Point play_ball_diff = ball_loc - play_loc;
	play_ball_diff = play_ball_diff.rotate(-orientationFromMatrix(dBodyGetRotation(dGeomGetBody(geom))));
	bool face = play_ball_diff.x >= x_len / 2 && (play_ball_diff.y * x_len) < (play_ball_diff.x * y_len);

	if (dGeomGetClass(geom) == dBoxClass && dGeomBoxPointDepth(geom, pos[0], pos[1], pos[2]) < 0) {
		return face;
	}
	return false;
}

//
//
//
void Compo_player_geom::handleRobotBallCollision(dGeomID o1, dGeomID o2, dJointGroupID contactgroup) {
	int i = 0;
	dBodyID b1 = dGeomGetBody(o1);
	dBodyID b2 = dGeomGetBody(o2);
	dContact contact[3];      // up to 3 contacts per box
	dGeomID ballGeom = o2;
	dGeomID robotGeom = o1;
	if (dGeomGetClass(o1) == dSphereClass) {
		robotGeom = o2;
		ballGeom = o1;
	}

	if (int numc = dCollide(o1, o2, 3, &contact[0].geom, sizeof(dContact))) {
		for (i = 0; i < numc; i++) {
			bool robotDribbler = dribblerBar == o1 || dribblerBar == o2;
			bool robotCollided = dBodyGetPosition(dGeomGetBody(ballGeom))[2] > DRIBBLER_HEIGHT && hasContactPenetration(contact[i].geom.pos, robotGeom);
			robotCollided = robotCollided || !hasContactWithFace(contact[i].geom.pos, robotGeom);
			has_ball_now = has_ball_now || robotDribbler;
			// different parameters are needed
			// for the dribbler
			if (robotDribbler) {
				contact[i].surface.mode = dContactSoftCFM | dContactSoftERP | dContactBounce;
				contact[i].surface.mu = 0.1;
				contact[i].surface.soft_cfm = CFM;
				contact[i].surface.soft_erp = ERP;
				contact[i].surface.bounce = 0.2;
				contact[i].surface.bounce_vel = 0.0;
				dJointID c = dJointCreateContact(world, contactgroup, contact + i);
				dJointAttach(c, b1, b2);
			} else if (robotCollided) {
				contact[i].surface.mode = dContactSoftCFM | dContactSoftERP | dContactBounce;
				contact[i].surface.mu = 0.1;
				contact[i].surface.soft_cfm = CFM;
				contact[i].surface.soft_erp = ERP;
				contact[i].surface.bounce = 0.2;
				contact[i].surface.bounce_vel = 0.0;
				dJointID c = dJointCreateContact(world, contactgroup, contact + i);
				dJointAttach(c, b1, b2);
			}
		}
	}
}

// dribbler == dGeomGetBody(o1) ||  dribbler == dGeomGetBody(o2)
// if a shape interescts with the ground set the contact parameters
//
void Compo_player_geom::handleCollisionWithGround(dGeomID o1, dGeomID o2, dJointGroupID contactgroup) {
	if (dGeomGetClass(o1) != dCCylinderClass && dGeomGetClass(o2) != dCCylinderClass) {
		dBodyID b1 = dGeomGetBody(o1);
		dBodyID b2 = dGeomGetBody(o2);
		dContact contact[3];      // up to 3 contacts per box
		if (int numc = dCollide(o1, o2, 3, &contact[0].geom, sizeof(dContact))) {
			for (int i = 0; i < numc; i++) {
				contact[i].surface.mode = dContactSoftCFM | dContactSoftERP | dContactBounce;
				contact[i].surface.mu = 0.0;
				contact[i].surface.soft_cfm = CFM;
				contact[i].surface.soft_erp = ERP;
				contact[i].surface.bounce = 0.8;
				contact[i].surface.bounce_vel = 0.0;
				dJointID c = dJointCreateContact(world, contactgroup, contact + i);
				dJointAttach(c, b1, b2);
			}
		}
	}
}

double orientationFromMatrix(const dReal *t) {
	return atan2(-t[1], t[0]);
}

//
// if ground or ball or wall isn't invloved, we assume a robot robot collision
//
void Compo_player_geom::handleRobotRobotCollision(dGeomID o1, dGeomID o2, dJointGroupID contactgroup) {
	const int num_contact = 4;
	if (dGeomGetBody(o1) != dGeomGetBody(o2)) {
		dBodyID b1 = dGeomGetBody(o1);
		dBodyID b2 = dGeomGetBody(o2);
		dContact contact[num_contact];        // up to 3 contacts per box
		if (int numc = dCollide(o1, o2, num_contact, &contact[0].geom, sizeof(dContact))) {
			for (int i = 0; i < numc; i++) {
				bool robotCollided = hasContactPenetration(contact[i].geom.pos, o1) && hasContactPenetration(contact[i].geom.pos, o2);
				if (robotCollided) {
					contact[i].surface.mode = dContactSoftCFM | dContactSoftERP | dContactBounce;
					contact[i].surface.mu = 0.1;
					contact[i].surface.soft_cfm = CFM;
					contact[i].surface.soft_erp = ERP;
					contact[i].surface.bounce = 1.0;
					contact[i].surface.bounce_vel = 0.0;
					dJointID c = dJointCreateContact(world, contactgroup, contact + i);
					dJointAttach(c, b1, b2);
				}
			}
		}
	}
}

void Compo_player_geom::dribble(double set_point) {
	// double voltage = set_point * MAX_VOLTAGE;
	// double rpm = RPM_PER_VOLT * voltage;
	// double rads_per_sec = (2*3.14/60)*rpm;
	// dJointSetAMotorParam (hinge, dParamVel, rads_per_sec);
}


// if a shape interescts with the wall set the contact parameters
//
void Compo_player_geom::handleWallCollision(dGeomID o1, dGeomID o2, dJointGroupID contactgroup) {
	int i = 0;
	dBodyID b1 = dGeomGetBody(o1);
	dBodyID b2 = dGeomGetBody(o2);

	dContact contact[3];      // up to 3 contacts per box
	if (int numc = dCollide(o1, o2, 3, &contact[0].geom, sizeof(dContact))) {
		for (i = 0; i < numc; i++) {
			contact[i].surface.mode = dContactSoftCFM | dContactSoftERP | dContactBounce | dContactApprox1;
			contact[i].surface.mu = 2.0;
			contact[i].surface.soft_cfm = CFM;
			contact[i].surface.soft_erp = ERP;
			contact[i].surface.bounce = 1.0;
			contact[i].surface.bounce_vel = 0.0;
			dJointID c = dJointCreateContact(world, contactgroup, contact + i);
			dJointAttach(c, b1, b2);
		}
	}
}

void Compo_player_geom::reset_frame() {
	has_ball_now = false;
}

bool Compo_player_geom::has_ball() const {
	return has_ball_now;
}

