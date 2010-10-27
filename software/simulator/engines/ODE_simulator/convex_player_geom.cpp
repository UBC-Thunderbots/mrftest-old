#include "convex_player_geom.h"
#include "geom/angle.h"
#include "geom/point.h"
#include <cmath>
#include <vector>

namespace {
	double orientationFromMatrix(const dReal *t) {
		return std::atan2(-t[1], t[0]);
	}

	const double CFM = 1E-5;

	const double ERP = 1.0;

	/**
	 * Number of sides used to generate the triangle mesh geometry
	 */
	const unsigned int NUM_SIDES = 20;

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


	const unsigned int CHOPPED_CIRCLE_APPROX = 15;
	const unsigned int NUM_POINTS = CHOPPED_CIRCLE_APPROX * 2; // number of points that define this convex hull
	const unsigned int PLANE_COUNT = CHOPPED_CIRCLE_APPROX + 2;

	dReal planes[PLANE_COUNT * 4];
	dReal points[3 * NUM_POINTS];
	unsigned int polygons[PLANE_COUNT + PLANE_COUNT * 4 + 2 * CHOPPED_CIRCLE_APPROX];
	const unsigned int POINT_COUNT = 30;

	std::vector<Point> get_chopped_circle(double radius, double face_width) {
		double face_depth = std::sqrt(radius * radius - (face_width / 2.0) * (face_width / 2.0));
		std::vector<Point> ans;

		Point first(face_depth, face_width / 2.0);
		Point last(face_depth, -face_width / 2.0);

		double angle_face = std::acos(first.cross(last) / (radius * radius));
		double angle_step = (2 * M_PI - angle_face) / (CHOPPED_CIRCLE_APPROX - 1);

		for (unsigned int i = 0; i < CHOPPED_CIRCLE_APPROX; i++) {
			ans.push_back(first.rotate(i * angle_step));
		}

		return ans;
	}

	dGeomID get_player_geom(dSpaceID space, double height, double radius, double face_width) {
		std::vector<Point> circle = get_chopped_circle(radius, face_width);
		// fill in the planes
		bool top = true;

		// fill in all of the planes
		for (unsigned int i = 0; i < PLANE_COUNT; i++) {
			if (i < circle.size()) { // either the face or the round part of the robot
				Point temp = circle[i] + circle[(i + 1) % circle.size()];
				temp /= 2;
				double len = temp.len();
				temp = temp.norm();
				planes[4 * i] = temp.x;
				planes[4 * i + 1] = temp.y;
				planes[4 * i + 2] = 0.0;
				planes[4 * i + 3] = len;
			} else if (top) { // the top part of the robot shape
				planes[4 * i] = 0.0;
				planes[4 * i + 1] = 0.0;
				planes[4 * i + 2] = 1.0;
				planes[4 * i + 3] = height / 2.0;
				top = false;
			} else { // the bottom part of the robot shape
				planes[4 * i] = 0.0;
				planes[4 * i + 1] = 0.0;
				planes[4 * i + 2] = -1.0;
				planes[4 * i + 3] = height / 2.0;
				top = true;
			}
		}

		// fill in the points
		for (unsigned int i = 0; i < NUM_POINTS; i++) {
			unsigned int circle_pt = i % circle.size();
			Point point_2d = circle[circle_pt];
			points[3 * i] = point_2d.x;
			points[3 * i + 1] = point_2d.y;

			if (i <= circle_pt) { // do top points first then bottom
				points[3 * i + 2] = height / 2;
			} else {
				points[3 * i + 2] = -height / 2;
			}
		}

		// fill in all the polygons
		int j = 0;
		for (unsigned int i = 0; i < PLANE_COUNT; i++) {
			if (i < circle.size()) { // either the face or the round part of the robot
				polygons[j++] = 4;
				polygons[j++] = i;
				polygons[j++] = i + circle.size();
				polygons[j++] = ((i + 1) % circle.size()) + circle.size();
				polygons[j++] = (i + 1) % circle.size();
			} else if (top) { // the top part of the robot shape
				polygons[j++] = circle.size();
				for (unsigned int k = 0; k < circle.size(); k++) {
					polygons[j++] = k;
				}
				top = false;
			} else { // the bottom part of the robot shape
				polygons[j++] = circle.size();
				for (int k = circle.size() - 1; k >= 0; k--) {
					polygons[j++] = k + circle.size();
				}
				top = true;
			}
		}
		return dCreateConvex(space, planes, PLANE_COUNT, points, POINT_COUNT, polygons);
	}
}

Convex_player_geom::Convex_player_geom(dWorldID eworld, dSpaceID dspace) : Player_geom(eworld, dspace) {
}

Convex_player_geom::~Convex_player_geom() {
}

bool Convex_player_geom::robot_contains_shape(dGeomID geom) {
	dBodyID b = dGeomGetBody(geom);
	return b == body;
}

bool Convex_player_geom::hasContactWithFace(dVector3 pos, dGeomID geom) {
	double y_len = 0.1;
	double x_len = std::sqrt((2 * ROBOT_RADIUS) * (2 * ROBOT_RADIUS) - (y_len) * (y_len));
	const dReal *p = dBodyGetPosition(dGeomGetBody(geom));
	Point ball_loc(pos[0], pos[1]);
	Point play_loc(p[0], p[1]);
	Point play_ball_diff = ball_loc - play_loc;
	play_ball_diff = play_ball_diff.rotate(-orientationFromMatrix(dBodyGetRotation(dGeomGetBody(geom))));
	bool face = play_ball_diff.x >= x_len / 2 && (play_ball_diff.y * x_len) < (play_ball_diff.x * y_len);

	return face;
}

void Convex_player_geom::handle_collision(dGeomID o1, dGeomID o2, dJointGroupID contactgroup) {
	bool ball_collision = false;
	double frict = 0.0;
	if (dGeomGetBody(o1) == dGeomGetBody(o2)) {
		return;
	}
	dGeomID robotGeom = o1;
	if (robot_contains_shape(o1)) {
		robotGeom = o2;
	}
	if (dGeomGetBody(o1) == 0 || dGeomGetBody(o2) == 0) {
		if (dGeomGetClass(o1) == dPlaneClass || dGeomGetClass(o2) == dPlaneClass) {
			frict = 0.0;
		} else {
			frict = 0.2;
		}
	} else if (dGeomGetClass(o1) == dSphereClass || dGeomGetClass(o2) == dSphereClass) {
		ball_collision = true;
		frict = 0.2;
	} else {
		frict = 0.2;
	}

	dContact contact[3];      // up to 3 contacts per box
	if (int numc = dCollide(o1, o2, 3, &contact[0].geom, sizeof(dContact))) {
		for (int i = 0; i < numc; i++) {
			if (ball_collision) {
				has_ball_now = has_ball_now || hasContactWithFace(contact[i].geom.pos, robotGeom);
			}
			contact[i].surface.mode = dContactSoftCFM | dContactSoftERP | dContactBounce;
			contact[i].surface.mu = frict;
			contact[i].surface.soft_cfm = CFM;
			contact[i].surface.soft_erp = ERP;
			contact[i].surface.bounce = 0.2;
			contact[i].surface.bounce_vel = 0.0;
			dJointID c = dJointCreateContact(world, contactgroup, contact + i);
			dJointAttach(c, dGeomGetBody(o1), dGeomGetBody(o2));
		}
	}
}

void Convex_player_geom::reset_frame() {
	has_ball_now = false;
}

bool Convex_player_geom::has_ball() const {
	return has_ball_now;
}

dGeomID Convex_player_geom::create_robot_geom() {
	return get_player_geom(space, ROBOT_HEIGHT, ROBOT_RADIUS, FRONT_FACE_WIDTH);
}

