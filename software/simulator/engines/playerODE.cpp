#include "simulator/engines/playerODE.h"
#include "world/timestep.h"
#include <iostream>
#include <math.h>
#include <algorithm>

			playerODE::playerODE (dWorldID eworld, dSpaceID dspace, dGeomID ballGeomi, double ups_per_tick) : the_position(0.0, 0.0), the_velocity(0.0, 0.0), target_velocity(0.0, 0.0), the_orientation(0.0), avelocity(0.0), target_avelocity(0.0), prevAccel(0.0, 0.0) {
			
			updates_per_tick = ups_per_tick;
				double dribble_radius = 0.005;//half a cm
				ballGeom = ballGeomi;
				double ballradius = dGeomSphereGetRadius(ballGeom);
				maxAvel = 2.0;
				maxAaccel = 1.0;
				world = eworld;
				body = dBodyCreate(world);
				body2 = dBodyCreate(world);
				double x_pos = 0.0;
				double y_pos = 0.0;
				jerkLimit = 30000.0;
				
				fcex=0;
				fcey=0;
				torquez=0;
				
				x_len = 0.18;
				y_len = 0.18;
				
				dBodySetPosition(body, x_pos, y_pos, 0.0006);
				dBodySetPosition(body2, x_pos, y_pos, 0.0515);
				dGeomID robotGeom = dCreateBox (0,x_len,y_len,0.001);//10cm 
				dGeomID robotGeomTop = dCreateBox (0,x_len,y_len,0.1);
				
				double arm_width = 0.001;
				double arm_height = 0.01;
				
				dGeomID dribbleArmL = dCreateBox (0,dribble_radius*2.5,arm_width,arm_height);
				dGeomID dribbleArmR = dCreateBox (0,dribble_radius*2.5,arm_width,arm_height);
				
				dMassSetSphere (&mass,0.5,0.267);
				dMassSetSphere (&mass2,0.1,0.267);
				dBodySetMass (body,&mass);
				dBodySetLinearDamping (body, 0.12);
				dBodySetMass (body2,&mass2);
				momentInertia = mass2.mass*0.2;
				dGeomSetBody (robotGeom,body);
				dGeomSetBody (robotGeomTop,body2);
				
				double arm_h_offset = ballradius - 0.051;
				
				dGeomSetBody (dribbleArmL,body2);
				dGeomSetBody (dribbleArmR,body2);
				
				dGeomSetOffsetPosition (dribbleArmL, x_len/2, y_len/2 + arm_width/2, arm_h_offset);
				dGeomSetOffsetPosition (dribbleArmR, -x_len/2, -y_len/2 - arm_width/2, arm_h_offset);
				
				dSpaceAdd (dspace, robotGeom);
				dSpaceAdd (dspace, robotGeomTop);
				dBodySetLinearDamping (body, 0.05);
				//dBodySetLinearDamping (body2, 0.05);
				dBodySetAngularDamping (body, 0.02);
				dBodySetAngularDamping (body2, 0.02);
				contactgroup = dJointGroupCreate (0);
	 			createJointBetweenB1B2();
			}
			
			playerODE::~playerODE () {
				dJointGroupDestroy (contactgroup);
				dBodyDestroy (body);
				dBodyDestroy (body2);
			}
			
			void playerODE::createJointBetweenB1B2(){
				dJointGroupDestroy (contactgroup);
				contactgroup = dJointGroupCreate(0);
				hinge=dJointCreateBall (world, contactgroup);
				  const dReal *t = dBodyGetPosition (body);
				  double x = t[0];
				  double y = t[1];
				  double z = t[2];
				  z+=0.0005;  
				dJointSetBallAnchor(hinge, x, y , z);
				dJointAttach (hinge, body, body2);
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

				bool hasTheBall = true;
				double hasBallTolerance = 0.0025;
				const dReal *b = dBodyGetPosition (dGeomGetBody(ballGeom)); 
				const dReal *p = dBodyGetPosition (body2);

				point ball_loc(b[0], b[1]);
				point play_loc(p[0], p[1]);
				point play_ball_diff = ball_loc - play_loc;
				point rel_play_ball_diff = play_ball_diff.rotate(-orientation());
				play_ball_diff  = rel_play_ball_diff;
				
				if(play_ball_diff.x < x_len/2 + dGeomSphereGetRadius(ballGeom) - hasBallTolerance){
					hasTheBall=false;
				}

				if(play_ball_diff.x > x_len/2 + dGeomSphereGetRadius(ballGeom) + hasBallTolerance){
					hasTheBall=false;
				}
				if(play_ball_diff.y > y_len/2 + dGeomSphereGetRadius(ballGeom) + hasBallTolerance){
					hasTheBall=false;
				}
				if(rel_play_ball_diff.x <0){
					hasTheBall=false;
				}
				double mag_y = abs(rel_play_ball_diff.y);
				double mag_x = abs(rel_play_ball_diff.x);

				if( mag_y/mag_x > y_len/x_len){
					hasTheBall=false;
				}

				return hasTheBall;
			}
			
			bool playerODE::has_ball(double tolerance){

				bool hasTheBall = true;
				double hasBallTolerance = tolerance;
				const dReal *b = dBodyGetPosition (dGeomGetBody(ballGeom)); 
				const dReal *p = dBodyGetPosition (body2);

				point ball_loc(b[0], b[1]);
				point play_loc(p[0], p[1]);
				point play_ball_diff = ball_loc - play_loc;
				point rel_play_ball_diff = play_ball_diff.rotate(-orientation());
				play_ball_diff  = rel_play_ball_diff;

				if(play_ball_diff.x < x_len/2 + dGeomSphereGetRadius(ballGeom) - hasBallTolerance){
					hasTheBall=false;
				}

				if(play_ball_diff.x > x_len/2 + dGeomSphereGetRadius(ballGeom) + hasBallTolerance){
					hasTheBall=false;
				}
				if(play_ball_diff.y > y_len/2 + dGeomSphereGetRadius(ballGeom) + hasBallTolerance){
					hasTheBall=false;
				}
				if(rel_play_ball_diff.x <0){
					hasTheBall=false;
				}
				double mag_y = abs(rel_play_ball_diff.y);
				double mag_x = abs(rel_play_ball_diff.x);

				if( mag_y/mag_x > y_len/x_len){
					hasTheBall=false;
				}

				return hasTheBall;
			}

			
			bool playerODE::robot_contains_shape(dGeomID geom){
				 dBodyID b = dGeomGetBody(geom);
				return (b==body)||(b==body2);
			}
			
			void playerODE::pre_tic(){

					dBodyAddTorque (body2, 0.0, 0.0, torquez);
					dBodyAddForce (body, fcex, fcey, 0.0);
				
			}

			void playerODE::move_impl(const point &vel, double avel) {					
				if(!posSet){
					double Accel_Max = 5.5;
					double V_MaxVel = 3.0;
					
					target_velocity = vel;
					point tVelocity = vel;
					target_avelocity = avel;
					avelocity = avel;

					const dReal *cur_vel = dBodyGetLinearVel(body);

					the_velocity.x=cur_vel[0];
					the_velocity.y= cur_vel[1];

					tVelocity = tVelocity;
					
				
					tVelocity = tVelocity.rotate(orientation());
					
					double magVel = sqrt(tVelocity.x*tVelocity.x + tVelocity.y*tVelocity.y);
					
					if(magVel>V_MaxVel){
						tVelocity= tVelocity/magVel;
						tVelocity= tVelocity*V_MaxVel;
					}
					
					dBodyEnable (body);
					dBodySetDynamic (body);

					point vDiff = tVelocity - the_velocity;				
					point acc = vDiff*static_cast<double>(TIMESTEPS_PER_SECOND);
					acc = acc/updates_per_tick;
					double magAcc = acc.len();
					
					if(magAcc>Accel_Max){
						acc=acc/magAcc;
						acc=acc*Accel_Max;
					}
					
					//acc
					point accelDiff = acc - prevAccel;
					
					double magJerk = accelDiff.len()*static_cast<double>(TIMESTEPS_PER_SECOND);
					
					double directionAccel_change = acc.dot(prevAccel);
					if(directionAccel_change>0){
					
					if(magJerk>jerkLimit){
						accelDiff = accelDiff/accelDiff.len();
						accelDiff = accelDiff*(jerkLimit/static_cast<double>(TIMESTEPS_PER_SECOND));
					}
					
					acc = prevAccel + accelDiff;
					
					}
					
					double m = mass.mass + mass2.mass;
					point fce = acc*((double)m);
					
					if(avel>maxAvel || avel<(-maxAvel)){
						//enorce a max turn speed
						if(avel>0){
							avelocity = maxAvel;
						}else{
							avelocity = -maxAvel;
						}
					}
					
					//std::cout<<"angular speed: "<<avel<<std::endl;
					const dReal * t =  dBodyGetAngularVel (body2);
					double avelRobot = t[2];
					//std::cout<<avelRobot<<std::endl;
					double avelDiff = avelocity - avelRobot;
					double aAccel = avelDiff*static_cast<double>(TIMESTEPS_PER_SECOND);
					aAccel = aAccel/updates_per_tick;
					
					if(aAccel>maxAaccel){
						aAccel= maxAaccel;
					}
					if(aAccel<(-maxAaccel)){
						aAccel= -maxAaccel;
					}

					double torque = aAccel*momentInertia;
					//double realizedAVel = avelRobot + aAccel;
										
					fcex=fce.x;
					fcey= fce.y;
					torquez=torque;
					//dBodySetAngularVel (body2, 0.0, 0.0, realizedAVel);
					dBodyAddTorque (body2, 0.0, 0.0, torque);

					dBodyAddForce (body, fce.x, fce.y, 0.0);
					
					prevAccel = acc;

				}
				posSet=false;		
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
				
				if(has_ball(0.012)){
				

				point ball_turn;
				ball_turn.x = t[0];
				ball_turn.y = t[1];
				if(! (ball_turn.len() > max_Angular_vel)){
				double forceMax = 0.0001;
								//std::cout<<"dribble"<<speed<<std::endl;
				//std::cout<<"dribble"<< t[0]<<" "<<t[1]<<" "<<t[2]<<std::endl;
					//dBodyAddTorque(dGeomGetBody(ballGeom), torqueAxis.x, torqueAxis.y, 0.0);
					point directionp(1,0);
					directionp = directionp.rotate(orientation());
					directionp = -directionp*forceMax*speed;
					dBodyAddForce(dGeomGetBody(ballGeom), directionp.x, directionp.y, 0.0);
					
				}
					
				}
				
				
			}

			void playerODE::kick(double strength) {

				if(strength <0 || strength >1)return;
				
				double maximum_impulse = 0.0001;
				point direction(1.0, 0.0);
				direction = direction.rotate(orientation());
				point impulse = strength*maximum_impulse*direction;

				if(has_ball(0.005)){
					dVector3 force;
					dWorldImpulseToForce (world, 1.0/static_cast<double>(TIMESTEPS_PER_SECOND),
				   			impulse.x, impulse.y,0.0, force);
				   	dBodyAddForce(dGeomGetBody(ballGeom), force[0], force[1], force[2]);
				}
			}

			void playerODE::chip(double strength) {

				if(strength <0 || strength >1)return;
				//std::cout<<"chip strength: "<<strength<<std::endl;
					double maximum_impulse = 0.0001;
					
				point direction(1.0/sqrt(2.0), 0.0);
				direction = direction.rotate(orientation());
				point impulse = strength*maximum_impulse*direction;
				
				double zimpulse = strength*maximum_impulse/sqrt(2.0);

				if(has_ball(0.01)){
					dVector3 force;
					dWorldImpulseToForce (world, 1.0/static_cast<double>(TIMESTEPS_PER_SECOND),
				   			impulse.x, impulse.y,zimpulse, force);
				   	dBodyAddForce(dGeomGetBody(ballGeom), force[0], force[1], force[2]);
				}
				//std::cout<<"chip strength: "<<strength<<std::endl;
			}

			void playerODE::ext_drag(const point &pos, const point &vel) {
				posSet = true;
				const dReal *t = dBodyGetPosition (body);
				const dReal *t2 = dBodyGetPosition (body2);
				dBodySetPosition(body, pos.x, pos.y, t[2]);
				dBodySetPosition(body2, pos.x, pos.y, t2[2]);
				dBodySetLinearVel(body,vel.x,vel.y,0.0);
				dBodySetLinearVel(body2,vel.x,vel.y,0.0);
				dBodySetAngularVel (body, 0.0, 0.0, 0.0);
				dBodySetAngularVel (body2, 0.0, 0.0, 0.0);
				createJointBetweenB1B2();

			}

		

