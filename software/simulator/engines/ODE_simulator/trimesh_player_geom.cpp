#include "trimesh_player_geom.h"
#include "geom/angle.h"
#include <cmath>


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
}

Trimesh_player_geom::Trimesh_player_geom(dWorldID eworld, dSpaceID dspace) : Player_geom(eworld, dspace), Triangles(0) {
}

Trimesh_player_geom::~Trimesh_player_geom() {
	delete[] Vertices;
	delete[] Verts;
	delete[] Triangles;
}

bool Trimesh_player_geom::robot_contains_shape(dGeomID geom) {
	dBodyID b = dGeomGetBody(geom);
	return b == body;
}


bool Trimesh_player_geom::hasContactWithFace(dVector3 pos, dGeomID geom) {
	double y_len = 0.1;
	double x_len = sqrt((2 * ROBOT_RADIUS) * (2 * ROBOT_RADIUS) - (y_len) * (y_len));
	const dReal *p = dBodyGetPosition(dGeomGetBody(geom));
	Point ball_loc(pos[0], pos[1]);
	Point play_loc(p[0], p[1]);
	Point play_ball_diff = ball_loc - play_loc;
	play_ball_diff = play_ball_diff.rotate(-orientationFromMatrix(dBodyGetRotation(dGeomGetBody(geom))));
	bool face = play_ball_diff.x >= x_len / 2 && (play_ball_diff.y * x_len) < (play_ball_diff.x * y_len);

	return face;
}

void Trimesh_player_geom::handle_collision(dGeomID o1, dGeomID o2, dJointGroupID contactgroup) {
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




void Trimesh_player_geom::reset_frame() {
	has_ball_now = false;
}

bool Trimesh_player_geom::has_ball() const {
	return has_ball_now;
}

dTriMeshDataID Trimesh_player_geom::create_robot_geom() {
	// Compute angle for front face (Cosine Law)
	double WideAngle = acos((FRONT_FACE_WIDTH * FRONT_FACE_WIDTH - 2 * ROBOT_RADIUS * ROBOT_RADIUS) / (-2 * ROBOT_RADIUS * ROBOT_RADIUS));

	// Compute remainder of angles
	double NarrowAngleSize = (2 * M_PI - WideAngle) / NUM_SIDES;

	// calculate the number of faces and vertices
	unsigned int NumVertices = (NUM_SIDES + 2) * 2;
	unsigned int NumTriangles = 4 * (NUM_SIDES + 1);

	double Angles[NUM_SIDES + 1];

	Vertices = new dVector3[NumVertices];
	Verts = new double[4 * NumVertices];
	// dVector3 Vertices[NumVertices];

	Triangles = new unsigned int[3 * NumTriangles];
	// unsigned int Triangles[3*NumTriangles];

	// Compute the angles
	Angles[0] = WideAngle / 2;
	for (unsigned int i = 0; i < NUM_SIDES; i++) {
		Angles[i + 1] = Angles[i] + NarrowAngleSize;
	}



	Vertices[0][0] = 0;
	Vertices[0][1] = 0;
	Vertices[0][2] = ROBOT_HEIGHT / 2;


	Vertices[NUM_SIDES + 2][0] = 0;
	Vertices[NUM_SIDES + 2][1] = 0;
	Vertices[NUM_SIDES + 2][2] = -ROBOT_HEIGHT / 2;

	for (unsigned int i = 0; i <= NUM_SIDES; i++) {
		Vertices[i + 1][0] = cos(Angles[i]) * ROBOT_RADIUS;
		Vertices[i + 1][1] = sin(Angles[i]) * ROBOT_RADIUS;
		Vertices[i + 1][2] = ROBOT_HEIGHT / 2;

		Vertices[i + 1 + NUM_SIDES + 2][0] = cos(Angles[i]) * ROBOT_RADIUS;
		Vertices[i + 1 + NUM_SIDES + 2][1] = sin(Angles[i]) * ROBOT_RADIUS;
		Vertices[i + 1 + NUM_SIDES + 2][2] = -ROBOT_HEIGHT / 2;
	}




	// Top Side
	unsigned int offset = 0;
	for (unsigned int i = 0; i < NUM_SIDES; i++) {
		Triangles[3 * (i + offset) + 0] = 0;
		Triangles[3 * (i + offset) + 1] = i + 1;
		Triangles[3 * (i + offset) + 2] = i + 2;
	}

	Triangles[3 * (NUM_SIDES + offset) + 0] = 0;
	Triangles[3 * (NUM_SIDES + offset) + 1] = NUM_SIDES + 1;
	Triangles[3 * (NUM_SIDES + offset) + 2] = 1;


	// Sides
	offset = offset + NUM_SIDES + 1;
	for (unsigned int i = 0; i < NUM_SIDES; i++) {
		Triangles[3 * (offset + i) + 0] = i + 1;
		Triangles[3 * (offset + i) + 1] = NUM_SIDES + 3 + i;
		Triangles[3 * (offset + i) + 2] = NUM_SIDES + 4 + i;
	}

	Triangles[3 * (offset + NUM_SIDES) + 0] = NUM_SIDES + 1;
	Triangles[3 * (offset + NUM_SIDES) + 1] = 2 * NUM_SIDES + 3;
	Triangles[3 * (offset + NUM_SIDES) + 2] = NUM_SIDES + 3;


	offset = offset + NUM_SIDES + 1;
	for (unsigned int i = 0; i < NUM_SIDES; i++) {
		Triangles[3 * (offset + i) + 0] = i + 1;
		Triangles[3 * (offset + i) + 1] = NUM_SIDES + 4 + i;
		Triangles[3 * (offset + i) + 2] = i + 2;
	}
	Triangles[3 * (offset + NUM_SIDES) + 0] = NUM_SIDES + 1;
	Triangles[3 * (offset + NUM_SIDES) + 1] = NUM_SIDES + 3;
	Triangles[3 * (offset + NUM_SIDES) + 2] = 1;


	// Bottom Side
	offset = offset + (NUM_SIDES + 1);
	for (unsigned int i = 0; i < NUM_SIDES; i++) {
		Triangles[3 * (offset + i) + 0] = NUM_SIDES + 2;
		Triangles[3 * (offset + i) + 1] = NUM_SIDES + 4 + i;
		Triangles[3 * (offset + i) + 2] = NUM_SIDES + 3 + i;
	}

	Triangles[3 * (NUM_SIDES + offset) + 0] = NUM_SIDES + 2;
	Triangles[3 * (NUM_SIDES + offset) + 1] = NUM_SIDES + 3;
	Triangles[3 * (NUM_SIDES + offset) + 2] = 2 * NUM_SIDES + 3;

	for (int i = 0; i < NumVertices; i++) {
		for (int j = 0; j < 4; j++) {
			Verts[4 * i + j] = Vertices[i][j];
		}
	}

	dTriMeshDataID triMesh;
	triMesh = dGeomTriMeshDataCreate();
	/*
	  Online manuel says:
	  void dGeomTriMeshDataBuildSimple (dTriMeshDataID g,
	                             const dVector3*Vertices, int VertexCount,
	                             const int* Indices, int IndexCount);

	  however, online forums indicate that the second parameter is actually a (*dReal)

	  see:

	  http://www.gamedev.net/community/forums/topic.asp?topic_id=319297

	 */
	dGeomTriMeshDataBuildSimple(triMesh, reinterpret_cast<dReal *>(Verts), NumVertices, Triangles, NumTriangles);

	return triMesh;
}

