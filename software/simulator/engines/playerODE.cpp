#include "simulator/engines/playerODE.h"
#include <iostream>
#include <math.h>

			playerODE::playerODE (dWorldID eworld, dSpaceID dspace) : the_position(0.0, 0.0), the_velocity(0.0, 0.0), target_velocity(0.0, 0.0), the_orientation(0.0), avelocity(0.0) {

				world = eworld;
	
			

			body = dBodyCreate(world);
			dBodySetPosition(body, 0.0, 1.0, 0.06);
			dGeomID robotGeom = dCreateBox (0,.1,.1,0.1);//10cm 

			dMassSetSphere (&mass,1,.9267);
			dBodySetMass (body,&mass);

			dGeomSetBody (robotGeom,body);
			dSpaceAdd (dspace, robotGeom);

			}




			point playerODE::position() const {
point p;
const dReal *t = dBodyGetPosition (body);
p.x = t[0];
p.y = t[1];

//std::cout<<"player"<<t[2]<<std::endl;
				return p;
			}

			double playerODE::orientation() const {
				return the_orientation;
			}

			bool playerODE::has_ball() const {
				return false;
			}

			void playerODE::move_impl(const point &vel, double avel) {

double V_DiffMax = 7;
double V_MaxVel = 2;

//std::cout<<"move impl"<<vel<<std::endl;
				target_velocity = vel;
				avelocity = avel;

const dReal *cur_vel = dBodyGetLinearVel(body);

the_velocity.x=cur_vel[0];
the_velocity.y= cur_vel[1];

target_velocity = target_velocity;

double temp = target_velocity.x;
target_velocity.x = -target_velocity.y;
target_velocity.y = temp;
target_velocity = target_velocity;

double magVel = sqrt(target_velocity.x*target_velocity.x + target_velocity.y*target_velocity.y);

if(magVel>V_MaxVel){
target_velocity= target_velocity/magVel;
target_velocity= target_velocity*V_MaxVel;
}

dBodyEnable (body);
dBodySetDynamic (body);

point zero(0.0,0.0);


point vDiff = target_velocity - the_velocity;


vDiff = vDiff/15.0;//timestep 5

double mag = sqrt(vDiff.x*vDiff.x + vDiff.y*vDiff.y);



point acc = vDiff;
if(mag>V_DiffMax){

acc=acc/mag;
acc=acc*V_DiffMax;
}

point fce = acc;



//dBodySetForce  (dBodyID b, dReal x, dReal y, 0);


dBodyAddForce (body, fce.x, fce.y, 0.0);

//mass



//dBodySetLinearVel (body, target_velocity.x, target_velocity.y, 0.0);

			}

			void playerODE::dribble(double speed) {
			}

			void playerODE::kick(double strength) {
			}

			void playerODE::chip(double strength) {
			}

			void playerODE::ui_set_position(const point &pos) {
			}

		

