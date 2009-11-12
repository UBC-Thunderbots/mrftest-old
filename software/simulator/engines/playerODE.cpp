#include "simulator/engines/playerODE.h"
#include <iostream>
#include <math.h>
#include <algorithm>

			playerODE::playerODE (dWorldID eworld, dSpaceID dspace) : the_position(0.0, 0.0), the_velocity(0.0, 0.0), target_velocity(0.0, 0.0), the_orientation(0.0), avelocity(0.0), target_avelocity(0.0) {

				world = eworld;
	
			

			body = dBodyCreate(world);
			
			body2 = dBodyCreate(world);
			
			double x_pos = 0.0;
			double y_pos = 0.0;
			
			dBodySetPosition(body, x_pos, y_pos, 0.0006);
			
			dBodySetPosition(body2, x_pos, y_pos, 0.0515);
			
			
			dGeomID robotGeom = dCreateBox (0,.1,.1,0.001);//10cm 
			dGeomID robotGeomTop = dCreateBox (0,.1,.1,0.1);
			
			
			
			dMassSetSphere (&mass,0.5,0.267);
			dMassSetSphere (&mass2,0.1,0.267);
			dBodySetMass (body,&mass);
			
			dBodySetLinearDamping (body, 0.02);
			
			dBodySetMass (body2,&mass2);

			dGeomSetBody (robotGeom,body);
			dGeomSetBody (robotGeomTop,body2);
			
			dSpaceAdd (dspace, robotGeom);
			dSpaceAdd (dspace, robotGeomTop);
			
			dBodySetAngularDamping (body, 0.2);
			dBodySetAngularDamping (body2, 0.2);
			
			contactgroup = dJointGroupCreate (0);


 			createJointBetweenB1B2();

			}
			
			void playerODE::createJointBetweenB1B2(){
			
			//if(hinge){
				//dJointDestroy (hinge);
				dJointGroupDestroy (contactgroup);
			//}
			
			contactgroup = dJointGroupCreate(0);
			//hinge=dJointCreateHinge (world, contactgroup);
			hinge=dJointCreateBall (world, contactgroup);
			  const dReal *t = dBodyGetPosition (body);
			  double x = t[0];
			  double y = t[1];
			  double z = t[2];
			  z+=0.0005;
			  
			  		//dJointSetHingeAnchor(hinge, x, y , z);
			  		dJointSetBallAnchor(hinge, x, y , z);
 			//dJointSetHingeAxis (hinge, 0.0, 0.0, 1.0);
			dJointAttach (hinge, body, body2);
			dVector3 result;
			dVector3 result2;

			
			 dJointEnable (hinge);
			  
			  
			  }
			
			
	
			
			

double positionFromMatrix(const dReal *t){

  double x1 = 1; double y1=0;
  double x2; double y2;

  double Rot_2d[2][2];
  Rot_2d[0][0]= t[0];
  Rot_2d[0][1]= t[1];
  Rot_2d[1][0]= t[4];
  Rot_2d[1][1]= t[5];

  x2 = Rot_2d[0][0]*x1 + Rot_2d[0][1]*y1;
  y2 = Rot_2d[1][0]*x1 + Rot_2d[1][1]*y1;

  point a(x1,y1);
  point b(x2,y2);

  double c = a.cross(b);
  double d = a.dot(b);
  double angle = acos(d);
  
  if(c<0){
  return -angle;
  }  
  return angle;
}



			point playerODE::position() const {
			  point p;
			  const dReal *t = dBodyGetPosition (body);
			  p.x = t[0];
			  p.y = t[1];
			  return p;
			}

			double playerODE::orientation() const {

			  const dReal *r = dBodyGetRotation(body);
			  positionFromMatrix(r);


			  double d = positionFromMatrix(dBodyGetRotation(body2));

				return d;
			}

			bool playerODE::has_ball() const {
				return false;
			}

void controllerHack(point &target_velocity){
 			double temp = target_velocity.x;
			target_velocity.x = -target_velocity.y;
			target_velocity.y = temp;
}

			void playerODE::move_impl(const point &vel, double avel) {

bool controller_not_nice_hack = false;

if(!posSet){
			double MaxRadians_perSec = 0.1;
			  double V_DiffMax = 1.25;
			  double V_MaxVel = .75;

			  target_velocity = vel;
			  avelocity = avel;

			  const dReal *cur_vel = dBodyGetLinearVel(body);

			  the_velocity.x=cur_vel[0];
			  the_velocity.y= cur_vel[1];

			  target_velocity = target_velocity;
if(controller_not_nice_hack == true){
controllerHack(target_velocity);
}else{
target_velocity = target_velocity.rotate(orientation());
}
			  double magVel = sqrt(target_velocity.x*target_velocity.x + target_velocity.y*target_velocity.y);
			  if(magVel>V_MaxVel){
			    target_velocity= target_velocity/magVel;
			    target_velocity= target_velocity*V_MaxVel;
			  }
			  dBodyEnable (body);
			  dBodySetDynamic (body);

			  point vDiff = target_velocity - the_velocity;
			  double mag = sqrt(vDiff.x*vDiff.x + vDiff.y*vDiff.y);

			  point acc = vDiff;
			  if(mag>V_DiffMax){
			    acc=acc/mag;
			    acc=acc*V_DiffMax;
			  }
			  double m = mass.mass;
			  point fce = acc*((double)m);
			  fce = fce/50;
			  
			  if(avel>0.1 || avel<-0.1){
			  
			  //enorce a max turn speed
			  if(avel>0){
			   avel = 0.1;
			  }else{
			   avel = -0.1;
			  }
			  
			  }
			  
		          dBodySetAngularVel (body2, 0.0, 0.0, avel);

			  dBodyAddForce (body, fce.x, fce.y, 0.0);
			  }
			  posSet=false;
			}

			void playerODE::dribble(double speed) {
			}

			void playerODE::kick(double strength) {
			}

			void playerODE::chip(double strength) {
			}

			void playerODE::ui_set_position(const point &pos) {
				posSet = true;
				const dReal *t = dBodyGetPosition (body);
				const dReal *t2 = dBodyGetPosition (body2);
				dBodySetPosition(body, pos.x, pos.y, t[2]);
				dBodySetPosition(body2, pos.x, pos.y, t2[2]);
				createJointBetweenB1B2();
			}

		

