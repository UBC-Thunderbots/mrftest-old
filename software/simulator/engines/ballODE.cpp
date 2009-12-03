#include "simulator/engines/ballODE.h"
#include <iostream>
/*

ballODE.h has the following:

public:
typedef Glib::RefPtr<ballODE> ptr;
dWorldID world;


private:
point the_position, the_velocity;


*/

			
			ballODE::ballODE(dWorldID dworld, dSpaceID dspace, double radius, double mass) : the_position(0.0, 0.0), the_velocity(0.0, 0.0) {

				world = dworld;
				dMass m;

				body = dBodyCreate(world);
				dBodySetPosition(body, 0.0, 0.0, radius + 0.01);
				ballGeom = dCreateSphere(0,radius);//golf ball radius 4.2672cm

				dMassSetSphere (&m,1.0,radius);
				dBodySetMass (body,&m);
				m.mass = mass;

				dGeomSetBody (ballGeom,body);
				dSpaceAdd (dspace, ballGeom);
				dBodySetLinearDamping (body, 0.2);
				 field::ptr fldd(new simulator_field);
				 fld = fldd;
				 
				 //dBodySetMaxAngularSpeed (body, 5.0);

			}
			
		
			ballODE::ballODE(dWorldID dworld, dSpaceID dspace, double radius): the_position(0.0, 0.0), the_velocity(0.0, 0.0){
			
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
			
			}
			
			ballODE::ballODE(dWorldID dworld, dSpaceID dspace): the_position(0.0, 0.0), the_velocity(0.0, 0.0){
			
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
			
			}

			ballODE::~ballODE(){



			}
			
			double ballODE::getRadius(){

				return dradius;
			}
			
			point ballODE::position() const {
				point p;
				const dReal *t = dBodyGetPosition (body);
				p.x = t[0];
				p.y = t[1];
				//std::cout<<"ball"<<t[2]<<std::endl;
				return p;
			}

			point ballODE::velocity() const {
				return the_velocity;
			}

			point ballODE::acceleration() const {

					return point(0.0, 0.0);
			}

			void ballODE::ext_drag(const point &pos, const point &vel) {
				const dReal *t = dBodyGetPosition (body);
				
				dBodySetPosition(body, pos.x, pos.y, t[2]);
				dBodySetLinearVel(body, vel.x, vel.y, 0.0);
				dBodySetAngularVel (body, 0.0, 0.0, 0.0);

			}

			bool ballODE::in_goal() {
			point p;
				const dReal *t = dBodyGetPosition (body);
				p.x = t[0];
				p.y = t[1];
				if(p.x>fld->length() || p.x<-fld->length()){
					if(p.y<fld->width() && p.y>-fld->goal_width()){
						//double height = t[2];
						//if(height<fld->goal_height())return true;
						return true;
					}
				}



				return false;
			}

