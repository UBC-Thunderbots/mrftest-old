#include "playerODE.h"
#include "util/timestep.h"
#include <iostream>
#include <math.h>
#include <algorithm>
#include "geom/angle.h"
#include <cassert>

namespace {

	double orientationFromMatrix(const dReal *t){
		return atan2(-t[1], t[0]);
	}
	
	//
	// The maximum acceleration of a robot, in metres per second squared.
	//
	//const double BOT_MAX_ACCELERATION = 10.0;

	//
	// The maximum velocity of a robot, in metres per second.
	//
	//const double BOT_MAX_VELOCITY = 5.0;
	
	//
	// The maximum angular velocity of a robot in radians per second
	//
	//const double BOT_MAX_A_VELOCITY = 5;
	
	//
	// The maximum angular acceleration of a robot, in radians per second squared
	//
	//const double BOT_MAX_A_ACCELERATION = 10.0;

	//
	// The acceleration due to friction against the ball, in metres per second squared.
	//
	//const double BALL_DECELERATION = 6.0;
	
	/// Conversion Factor from the value used in radio packets (1/4 degree) per 5 ms to motor voltage
	const double PACKET_TO_VOLTAGE = 0.022281639;
	
	
	/// Maximum voltage available to the robot.
	const double VOLTAGE_LIMIT = 15.0;
	
	
	/// Resistance of the stator
	const double MOTOR_RESISTANCE = 1.2; //ohms
	
	/// Constant relating motor current to motor torque
	const double CURRENT_TO_TORQUE = 0.0255; //Nm / amp
	
	/// Gearing ration of the drive train (speed reduction, torque increase)
	const double GEAR_RATIO = 3.5;

	/// physical radius of the robot in meters
	const double ROBOT_RADIUS = 0.09;
	
	/// Physical mass of the robot in kg (measured)
	const double ROBOT_MASS = 2.552;
	
	/// Physical height of the robot in m
	const double ROBOT_HEIGHT = 0.15;
	
	/// Physical width of the flattened face of the robot
	const double FRONT_FACE_WIDTH = 0.16;
	
	/// Number of sides used to generate the triangle mesh geometry
	const unsigned int NUM_SIDES = 20;
	
	
	/// Angles in radians that the wheels are located off the forward direction
	const double ANGLES[4] = {0.959931, 2.35619, 3.9269908, 5.32325}; 
	
	int click =0;
}

/**
	Constructor method for the robot model contained in the simulator
*/
playerODE::playerODE (dWorldID eworld, dSpaceID dspace, dGeomID ballGeomi, double ups_per_tick) : target_velocity(0.0, 0.0), Vertices(0), Triangles(0) {

	updates_per_tick = ups_per_tick;
	double dribble_radius = 0.005;//half a cm
	ballGeom = ballGeomi;
	double ballradius = dGeomSphereGetRadius(ballGeom);

	world = eworld;
	body = dBodyCreate(world);
	double x_pos = 0.0;
	double y_pos = 0.0;
	jerkLimit = 30000.0;

	fcex=0;
	fcey=0;
	torquez=0;

	x_len = 0.18;
	y_len = 0.18;

	
	y_len = 0.1;
	x_len = sqrt((2*ROBOT_RADIUS)*(2*ROBOT_RADIUS) -(y_len)*(y_len));

	//dBodySetPosition(body, x_pos, y_pos, 0.0006);
	//robotGeomTop = dCreateTriMesh(0,create_robot_geom(),NULL,NULL,NULL);		
	robotGeomTop = dCreateBox (0,x_len,y_len,ROBOT_HEIGHT);
	dMassSetCylinderTotal (&mass,ROBOT_MASS, 3,ROBOT_RADIUS,ROBOT_HEIGHT);
	dBodySetMass (body,&mass);
	dGeomSetBody (robotGeomTop,body);

//ROBOT_RADIUS
//comment
	robotGeomTopCyl = dCreateCapsule (0, ROBOT_RADIUS, ROBOT_HEIGHT);
	dGeomSetBody (robotGeomTopCyl,body);
	dBodySetPosition(body, x_pos, y_pos, ROBOT_HEIGHT/2 + 0.001);
	//dGeomID robotGeom = dCreateBox (0,x_len,y_len,0.001);//10cm 
	
	double arm_width = 0.001;
	double arm_height = 0.01;

	dribbleArmL = dCreateBox (0,dribble_radius*2.5,arm_width,arm_height);
	dribbleArmR = dCreateBox (0,dribble_radius*2.5,arm_width,arm_height);

	
	
	momentInertia = ROBOT_RADIUS*ROBOT_RADIUS*mass.mass/2;
	//dGeomSetBody (robotGeom,body);
	
	double arm_h_offset = ballradius - 0.051;

	dGeomSetBody (dribbleArmL,body);
	dGeomSetBody (dribbleArmR,body);

	dGeomSetOffsetPosition (dribbleArmL, x_len/2, y_len/2 + arm_width/2, arm_h_offset);
	dGeomSetOffsetPosition (dribbleArmR, -x_len/2, -y_len/2 - arm_width/2, arm_h_offset);

	dSpaceAdd (dspace, robotGeomTop);
	dSpaceAdd (dspace, robotGeomTopCyl);
	dSpaceAdd (dspace, dribbleArmL);
	dSpaceAdd (dspace, dribbleArmR);


		wheel_position = new point[4];
		force_direction = new point[4];
		
	for(int index=0;index<4;index++)
	{
		wheel_position[index]=point(1,0).rotate(ANGLES[index])*ROBOT_RADIUS;
		force_direction[index]=point(wheel_position[index].rotate(PI/2).norm());
	}
	
}



playerODE::~playerODE () {
	//dJointGroupDestroy (contactgroup);
	dGeomDestroy(robotGeomTop);
	dGeomDestroy(robotGeomTopCyl);
	dGeomDestroy(dribbleArmL);
	dGeomDestroy(dribbleArmR);
	dBodyDestroy (body);
	

	if(Vertices != NULL)
		delete Vertices;
	if(Triangles != NULL)
		delete Triangles;
	
	if(wheel_position != NULL)
		delete[] wheel_position;
	
	if(force_direction != NULL)
		delete[] force_direction;
		
	//dBodyDestroy (body2);
}
/**
this should only be called from simulator during collision detection
*/
void playerODE::set_has_ball(){

//std::cout<<"set has ball"<<std::endl;
player_has_ball = true;
}

/**
used to decide whether to add a contact joint for a robot collision
*/
bool playerODE::hasContactPenetration(dVector3 pos){
	//if(dGeomBoxPointDepth (dGeomID box, dReal x, dReal y, dReal z);
	//((GeomBox)
	// robotGeomTop).pointDepth(pos)<0
	if(dGeomBoxPointDepth (robotGeomTop, pos[0], pos[1], pos[2])<0){
		return true;
	}


	const dReal *p = dBodyGetPosition (body);

	point ball_loc(pos[0], pos[1]);
	point play_loc(p[0], p[1]);
	point play_ball_diff = ball_loc - play_loc;
	play_ball_diff = play_ball_diff.rotate(-orientation());

	bool face = play_ball_diff.x>=x_len/2 && (play_ball_diff.y*x_len) <= (play_ball_diff.x*y_len);

	if(!face && pos[2]>0 && pos[2]<ROBOT_HEIGHT){
		return (dGeomCapsulePointDepth (robotGeomTopCyl, pos[0], pos[1], pos[2])<0);
	}

	return false;
}

/**
sees whether the contact joint penetrates the face of the robot
*/
bool playerODE::hasContactWithFace(dVector3 pos){
	const dReal *p = dBodyGetPosition (body);
	point ball_loc(pos[0], pos[1]);
	point play_loc(p[0], p[1]);
	point play_ball_diff = ball_loc - play_loc;
	play_ball_diff = play_ball_diff.rotate(-orientation());
	bool face = play_ball_diff.x>=x_len/2 && (play_ball_diff.y*x_len) < (play_ball_diff.x*y_len);

	if(dGeomBoxPointDepth (robotGeomTop, pos[0], pos[1], pos[2])<0){
		return face;
	}
	return false;
}


/// Accessor method to get the robots position
point playerODE::position() const {
	const dReal *t = dBodyGetPosition (body);
	return point(t[0], t[1]);
}


/// Accessor method to get the robots orientation
double playerODE::orientation() const {
	return orientationFromMatrix(dBodyGetRotation(body));
}


/**
Returns whether or not a given robot has the ball. 
Has ball is determined from the collision detection from the previous timestep
*/
bool playerODE::has_ball() const {	
	return player_has_ball;
}


/// Accessor to get the height of the middle of the robot (should be ROBOT_HEIGHT/2)
double playerODE::get_height() const
{
	const dReal *t = dBodyGetPosition (body);
	return t[2];
}


bool playerODE::robot_contains_shape(dGeomID geom){
//if(geom==dribbleArmL){
//std::cout<<"left Arm collide"<<std::endl;
//}

	dBodyID b = dGeomGetBody(geom);
	return (b==body);
}
//robot_contains_shape_ground(dGeomID geom)

bool playerODE::robot_contains_shape_ground(dGeomID geom){


	dBodyID b = dGeomGetBody(geom);
	return (b==body) && (geom!=robotGeomTopCyl);
}

//paramter is timestep
void playerODE::pre_tic(double ){

click++;

	//Current Motor Speed
	double motor_current[4];
	double wheel_torque;
	point force;
	

	//player doesn't have ball
	//unless we determine from collision detection
	//that the player does have the ball

	
	if(!posSet){
		
		//get the current bots velocity	
		const dReal *cur_vel = dBodyGetLinearVel(body);
		point the_velocity(cur_vel[0], cur_vel[1]);
		
		//rotate it to bot relative
		the_velocity = the_velocity.rotate(-orientation());
		
		//get the angular velocity
		const dReal * avels =  dBodyGetAngularVel (body);
		
		//convert the velocities to packet data type for comparison to stored values
		motor_current[0] = -42.5995*the_velocity.x + 27.6645*the_velocity.y + 4.3175 * avels[2]; 
		motor_current[1] = -35.9169*the_velocity.x + -35.9169*the_velocity.y + 4.3175*avels[2];
		motor_current[2] = 35.9169*the_velocity.x + -35.9169*the_velocity.y  + 4.3175*avels[2];
		motor_current[3] = 42.5995*the_velocity.x + 27.6645*the_velocity.y + 4.3175*avels[2];		
		
		dBodyEnable (body);
		dBodySetDynamic (body);
		
		
		for(int index=0;index<4;index++)
		{
			//motor desired in this context should be coming from the firmware interpreter
			wheel_torque=(motor_desired[index]-motor_current[index])*PACKET_TO_VOLTAGE/MOTOR_RESISTANCE*CURRENT_TO_TORQUE*GEAR_RATIO;
			force = force_direction[index]*wheel_torque/0.0254; //scale by wheel radius
			
			//Adds the force at the wheel positions, at mid-point of the robot(not realistic but should prevent tipping that we can't detect)
			dBodyAddRelForceAtRelPos(body, force.x, force.y, ROBOT_HEIGHT/2, wheel_position[index].x, wheel_position[index].y, ROBOT_HEIGHT/2);
		}
		
		if(chip_set){
			if(execute_chip()){
				chip_set=false;
			}
		}else if(kick_set){
			if(execute_kick()){
				chip_set=false;
			}
		}

	}
		player_has_ball=false;
	posSet=false;
}


//received data from ai does some checks and stores it,
//when implemented should pass to firmware interpreter
void playerODE::move_impl(const point &vel, double avel) {					
			
		point new_vel = vel;
	
	//These are used directly in the simulator code, needs to intercepted by a 
	//firmware intepreter to simulate controller			
	motor_desired[0] = -42.5995*new_vel.x +  27.6645*new_vel.y + 4.3175*avel; 
	motor_desired[1] = -35.9169*new_vel.x + -35.9169*new_vel.y + 4.3175*avel;
	motor_desired[2] =  35.9169*new_vel.x + -35.9169*new_vel.y + 4.3175*avel;
	motor_desired[3] =  42.5995*new_vel.x +  27.6645*new_vel.y + 4.3175*avel;		
	
	
	//limit max motor "voltage" to VOLTAGE_LIMIT by scaling the largest component to VOLTAGE_LIMIT if greater
	for(int index=0;index<4;index++)
		if(fabs(motor_desired[index])>VOLTAGE_LIMIT/PACKET_TO_VOLTAGE)
			for(int index2=0;index2<4;index2++)
				motor_desired[index2]=motor_desired[index2]/motor_desired[index]*VOLTAGE_LIMIT/PACKET_TO_VOLTAGE;
}



void playerODE::dribble(double speed) {

	double max_Angular_vel = 5.0;

	if(speed<0 || speed>1)return;

	double maxTorque = 0.00001;//static_cast<double>(TIMESTEPS_PER_SECOND); //is this realistic???			
	double appliedTorque = -(speed*maxTorque);

	const dReal * t = dBodyGetAngularVel (dGeomGetBody(ballGeom));
	//std::cout<<"dribble"<< t[0]<<" "<<t[1]<<" "<<t[2]<<std::endl;

	//std::cout<<"dribble speed: "<<speed<<std::endl;
	point torqueAxis(0,1);
	torqueAxis = torqueAxis.rotate(orientation());

	torqueAxis*=appliedTorque;

	if(has_ball()){


		point ball_turn;
		ball_turn.x = t[0];
		ball_turn.y = t[1];
		if(! (ball_turn.len() > max_Angular_vel)){
			//double forceMax = 0.1;
			//std::cout<<"dribble"<<speed<<std::endl;
			//std::cout<<"dribble"<< t[0]<<" "<<t[1]<<" "<<t[2]<<std::endl;
			//dBodyAddTorque(dGeomGetBody(ballGeom), torqueAxis.x, torqueAxis.y, 0.0);
			//point directionp(1,0);
			//directionp = directionp.rotate(orientation());
			//directionp = -directionp*forceMax*speed;
			//dBodyAddForce(dGeomGetBody(ballGeom), directionp.x, directionp.y, 0.0);

		}
//			double forceMax = 0.1;
//			std::cout<<"dribble"<<speed<<std::endl;
//			point directionp(1,0);
//			directionp = directionp.rotate(orientation());
//			directionp = -directionp*forceMax*speed;
//			dBodyAddForce(dGeomGetBody(ballGeom), directionp.x, directionp.y, 0.0);

	}
}

void playerODE::kick(double strength) {
	
	if(has_ball()){
		kick_strength = strength;
		kick_set = true;
	}

}

bool playerODE::execute_kick() {

	double strength = kick_strength;
	double maximum_impulse = 1.0;
	point direction(1.0, 0.0);
	direction = direction.rotate(orientation());
	point impulse = strength*maximum_impulse*direction;
	double zimpulse = strength*maximum_impulse/sqrt(2.0);

	if(has_ball()){
	 	std::cout<<"attempting chipping"<<robotGeomTop<<" "<<click<<std::endl;
		dVector3 force;

		dWorldImpulseToForce (world, 1.0/(static_cast<double>(TIMESTEPS_PER_SECOND)*updates_per_tick),
				impulse.x, impulse.y,zimpulse, force);

		dBodyAddForce(dGeomGetBody(ballGeom), force[0], force[1], force[2]);
	}
	return has_ball();
	//return true;

}

void playerODE::chip(double strength) {


	if(has_ball()){
	chip_strength = strength;
	chip_set = true;
	}

}

bool playerODE::execute_chip() {

	double strength = chip_strength;
	//std::cout<<"chip strength: "<<strength<<std::endl;
	double maximum_impulse = 1.0;

	point direction(1.0/sqrt(2.0), 0.0);
	direction = direction.rotate(orientation());
	point impulse = strength*maximum_impulse*direction;

	double zimpulse = strength*maximum_impulse/sqrt(2.0);

	if(has_ball()){
	 	std::cout<<"attempting chipping"<<robotGeomTop<<" "<<click<<std::endl;
		dVector3 force;

		dWorldImpulseToForce (world, 1.0/(static_cast<double>(TIMESTEPS_PER_SECOND)*updates_per_tick),
				impulse.x, impulse.y,zimpulse, force);

		dBodyAddForce(dGeomGetBody(ballGeom), force[0], force[1], force[2]);
		
	}
	
	return has_ball();
	

}

void playerODE::position(const point &pos) {
	posSet = true;
	dBodySetPosition(body, pos.x, pos.y, ROBOT_HEIGHT/2+0.01);
}

void playerODE::velocity(const point &vel) {
	dBodySetLinearVel(body,vel.x,vel.y,0.0);
}

void playerODE::orientation(double orient) {
	posSet=true;
	dMatrix3 RotationMatrix;
	dRFromAxisAndAngle (RotationMatrix, 0.0, 0.0, 1.0, orient);
	dBodySetRotation (body, RotationMatrix);
}

void playerODE::avelocity(double avel) {
	dBodySetAngularVel (body, 0.0, 0.0, avel);
}

dTriMeshDataID playerODE::create_robot_geom()
{

	//Compute angle for front face (Cosine Law)
	double WideAngle = acos((FRONT_FACE_WIDTH*FRONT_FACE_WIDTH - 2*ROBOT_RADIUS*ROBOT_RADIUS)/(-2*ROBOT_RADIUS*ROBOT_RADIUS));

	//Compute remainder of angles
	double NarrowAngleSize = (2*PI - WideAngle)/NUM_SIDES;

	//calculate the number of faces and vertices
	unsigned int NumVertices=(NUM_SIDES+2)*2;
	unsigned int NumTriangles=4*(NUM_SIDES+1);

	double Angles[NUM_SIDES+1];

	Vertices = new dVector3[NumVertices];
	
	//dVector3 Vertices[NumVertices];
	
	Triangles = new unsigned int[3*NumTriangles];
	//unsigned int Triangles[3*NumTriangles];

	//Compute the angles
	Angles[0]=WideAngle/2;
	for(unsigned int i=0;i<NUM_SIDES;i++)
	    Angles[i+1]=Angles[i]+NarrowAngleSize;



	Vertices[0][0]=0; 
	Vertices[0][1]=0;
	Vertices[0][2]=ROBOT_HEIGHT/2;
	 

	Vertices[NUM_SIDES+2][0]=0; 
	Vertices[NUM_SIDES+2][1]=0; 
	Vertices[NUM_SIDES+2][2]=-ROBOT_HEIGHT/2;

	for(unsigned int i=0;i<=NUM_SIDES;i++)
	{
	    Vertices[i+1][0]=cos(Angles[i])*ROBOT_RADIUS; 
	    Vertices[i+1][1]=sin(Angles[i])*ROBOT_RADIUS; 
	    Vertices[i+1][2]=ROBOT_HEIGHT/2;

	    Vertices[i+1+NUM_SIDES+2][0]=cos(Angles[i])*ROBOT_RADIUS; 
	    Vertices[i+1+NUM_SIDES+2][1]=sin(Angles[i])*ROBOT_RADIUS;
	    Vertices[i+1+NUM_SIDES+2][2]=-ROBOT_HEIGHT/2;
	}




	//Top Side
	unsigned int offset=0;
	for(unsigned int i=0;i<NUM_SIDES;i++)
	{
		Triangles[3*(i+offset)+0]=0; 
		Triangles[3*(i+offset)+1]=i+1; 
		Triangles[3*(i+offset)+2]=i+2;
	}

	Triangles[3*(NUM_SIDES+offset)+0]=0;
	Triangles[3*(NUM_SIDES+offset)+1]=NUM_SIDES+1; 
	Triangles[3*(NUM_SIDES+offset)+2]=1;


	//Sides
	offset=offset+NUM_SIDES+1;
	for(unsigned int i=0;i<NUM_SIDES;i++)
	{
		Triangles[3*(offset+i)+0]=i+1;
		Triangles[3*(offset+i)+1]=NUM_SIDES+3+i; 
		Triangles[3*(offset+i)+2]=NUM_SIDES+4+i;
	}

	Triangles[3*(offset+NUM_SIDES)+0]=NUM_SIDES+1;
	Triangles[3*(offset+NUM_SIDES)+1]=2*NUM_SIDES+3;
	Triangles[3*(offset+NUM_SIDES)+2]=NUM_SIDES+3;


	offset=offset+NUM_SIDES+1;
	for(unsigned int i=0;i<NUM_SIDES;i++)
	{
		Triangles[3*(offset+i)+0]=i+1;
		Triangles[3*(offset+i)+1]=NUM_SIDES+4+i;
		Triangles[3*(offset+i)+2]=i+2;
	}
	Triangles[3*(offset+NUM_SIDES)+0]=NUM_SIDES+1;
	Triangles[3*(offset+NUM_SIDES)+1]=NUM_SIDES+3; 
	Triangles[3*(offset+NUM_SIDES)+2]=1;


	//Bottom Side
	offset=offset+(NUM_SIDES+1);
	for(unsigned int i=0;i<NUM_SIDES;i++)
	{
		Triangles[3*(offset+i)+0]=NUM_SIDES+2;
		Triangles[3*(offset+i)+1]=NUM_SIDES+4+i;
		Triangles[3*(offset+i)+2]=NUM_SIDES+3+i;
	}

	Triangles[3*(NUM_SIDES+offset)+0]=NUM_SIDES+2;
	Triangles[3*(NUM_SIDES+offset)+1]=NUM_SIDES+3;
	Triangles[3*(NUM_SIDES+offset)+2]=2*NUM_SIDES+3;

	dTriMeshDataID triMesh;
	triMesh = dGeomTriMeshDataCreate();
	dGeomTriMeshDataBuildSimple(triMesh,Vertices[0],NumVertices,Triangles,NumTriangles);

	return triMesh;

}

/*
	struct __attribute__((packed)) RUN_DATA {
		uint8_t flags;
		signed drive1_speed : 11;
		signed drive2_speed : 11;
		signed drive3_speed : 11;
		signed drive4_speed : 11;
		signed dribbler_speed : 11;
		unsigned chick_power : 9;
	};
	const uint8_t RUN_FLAG_RUNNING = 0x80;
	const uint8_t RUN_FLAG_DIRECT_DRIVE = 0x01;
	const uint8_t RUN_FLAG_CONTROLLED_DRIVE = 0x02;
	const uint8_t RUN_FLAG_CHICKER_ENABLED = 0x04;
	const uint8_t RUN_FLAG_CHIP = 0x08;
	const uint8_t RUN_FLAG_FEEDBACK = 0x40;
*/

void playerODE::received(const xbeepacket::RUN_DATA &packet) {
	
	assert(packet.flags&xbeepacket::RUN_FLAG_RUNNING);
	
	// These two aren't used for now because we don't have a firmware simulator
	direct_drive = packet.flags&xbeepacket::RUN_FLAG_DIRECT_DRIVE;
	controlled_drive = packet.flags&xbeepacket::RUN_FLAG_CONTROLLED_DRIVE;

	

	// These settings are being passed directly for now, we should probably put code in to handle the fact that packets can arrive whenever
	// and that we could get more than one kick packet
	if(packet.flags&xbeepacket::RUN_FLAG_CHICKER_ENABLED) {
		if(packet.flags&xbeepacket::RUN_FLAG_CHIP){
			chip(packet.chick_power);
		
		}else{
			kick(packet.chick_power);
		}
	}

	/// The dribbler code will probably be re-written so this will do for now.
	dribble(packet.dribbler_speed);
	


	//These are used directly in the simulator code, needs to intercepted by a 
	//firmware intepreter to simulate controller			
	motor_desired[0] = packet.drive1_speed;
	motor_desired[1] = packet.drive2_speed;
	motor_desired[2] = packet.drive3_speed;
	motor_desired[3] = packet.drive4_speed;
	
	
	//limit max motor "voltage" to VOLTAGE_LIMIT by scaling the largest component to VOLTAGE_LIMIT if greater
	for(int index=0;index<4;index++)
		if(fabs(motor_desired[index])>VOLTAGE_LIMIT/PACKET_TO_VOLTAGE)
			for(int index2=0;index2<4;index2++)
				motor_desired[index2]=motor_desired[index2]/motor_desired[index]*VOLTAGE_LIMIT/PACKET_TO_VOLTAGE;



}



























