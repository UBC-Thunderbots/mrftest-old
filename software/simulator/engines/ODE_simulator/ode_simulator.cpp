#include "simulator/field.h"
#include "simulator/engines/engine.h"
#include "ballODE.h"
#include "playerODE.h"
#include "util/timestep.h"
#include "geom/angle.h"
#include <iostream>

#define MU 0.02		// the global mu to use
namespace {

	//
	// The limit of floating-point precision.
	//
	const double EPS = 1.0e-9;

	//
	// The force of gravity N/kg
	//
	const double GRAVITY = -9.81;
	
	
	const unsigned int UPDATES_PER_TICK = 500;

	//
	//
	//
	const double CFM = 1E-5;
	
	//
	//
	//
	const double ERP = 1.0;
	

	//
	// A SimulatorEngine.
	//
	class SimEngine : public SimulatorEngine {
		private:
			BallODE::ptr the_ball;
			std::vector<PlayerODE::ptr> the_players;
			PlayerODE::ptr emptyPlayer;
		public:
			double timeStep;
			dWorldID eworld;
			dSpaceID space;
			dGeomID ground;
			dGeomID wall[4];
			dJointGroupID contactgroup;
			//int stepSize;
			
			bool isWall(dGeomID geom){
				for(int i=0; i<4; i++){
					if(geom == wall[i])return true;
				}
				return false;
			}

			SimEngine(){
				dInitODE();
				timeStep = 1.0/(static_cast<double>(TIMESTEPS_PER_SECOND)*static_cast<double>(UPDATES_PER_TICK));
				eworld = dWorldCreate(); 
				dWorldSetGravity (eworld,0,0.0,GRAVITY);
				space = dSimpleSpaceCreate(0);
				//space = dHashSpaceCreate(0);
				
  				ground = dCreatePlane (space,0,0,1,0);
  				//wall[0] = dCreatePlane(space,1,0,0,fld->total_length()/2);
  				//wall[1] = dCreatePlane(space,1,0,0,-fld->total_length()/2);
  				//wall[2] = dCreatePlane(space,0,1,0,fld->total_width()/2);
  				//wall[3] = dCreatePlane(space,0,1,0,-fld->total_width()/2);
  				
  				
  				
  				double wall_height = 20.5; //1/2 meter
  				double wall_thickness = 0.0127; //
  				
				//build a wall around the playing field
  				wall[0] = dCreateBox (space, SimulatorField::total_length + 2*wall_thickness, wall_thickness, wall_height);
				wall[1] = dCreateBox (space, SimulatorField::total_length + 2*wall_thickness, wall_thickness, wall_height);  			
				wall[2] = dCreateBox (space, wall_thickness, SimulatorField::total_width-2*wall_thickness, wall_height);
				wall[3] = dCreateBox (space, wall_thickness, SimulatorField::total_width-2*wall_thickness, wall_height);
  				dGeomSetPosition (wall[0],  0,  (SimulatorField::total_width/2 + wall_thickness/2),  (wall_height/2));
  				dGeomSetPosition (wall[1],  0, -(SimulatorField::total_width/2 + wall_thickness/2),  (wall_height/2));
  				dGeomSetPosition (wall[2],  (SimulatorField::total_length/2 + wall_thickness/2), 0,  (wall_height/2));
  				dGeomSetPosition (wall[3], - (SimulatorField::total_length/2 + wall_thickness/2), 0,  (wall_height/2));
				//set possible penetration for collisions
    				
    				dWorldSetContactSurfaceLayer(eworld, 0.1);
				contactgroup = dJointGroupCreate (0);

				BallODE::ptr b(new BallODE(eworld, space));
				the_ball = b;
				
 				//dWorldSetLinearDamping (eworld, 0.02);
				dWorldSetCFM (eworld, CFM);
				
				 
 				
			}
			
			~SimEngine(){
				the_players.clear();
 				the_ball.reset();
 				dWorldDestroy (eworld);
 				dJointGroupDestroy (contactgroup);
 				dCloseODE();
			}


			void tick() {	
				for(unsigned int i=0; i< UPDATES_PER_TICK; i++){	
						for (unsigned int j = 0; j < the_players.size(); j++) {
							the_players[j]->pre_tic(timeStep);
							
						}
					//std::cout << "Player: " << the_players[0]->get_height() << ": The Ball: " << the_ball->get_height() << std::endl;
					//std::cout<<"tick Start"<<std::endl;
					//check the World for possible collisions
					//if there are colliding objects then call nearCallback
					//nearCallback creates all necessary contact points and parameters
	 				dSpaceCollide (space,this,&SimEngine::nearCallbackThunk);
	 				
	 				//step the World (have ODE do 1 iterations per step)
					//dWorldStep (eworld, 1);
					dWorldSetQuickStepNumIterations (eworld, 50);
					
					//double timeStep = 1.0/static_cast<double>(UPDATES_PER_TICK);
					
					dWorldQuickStep(eworld, timeStep);
					//dWorldStep(eworld,timeStep);
					
					//remove all the contact points that we created in this step
					dJointGroupEmpty (contactgroup);
					//std::cout<<"tick End"<<std::endl;
				
				}
			}
			void setWorld(dWorldID world) {
				eworld = world;
			}
			SimulatorBall::ptr get_ball() {
				return the_ball;
			}

			SimulatorPlayer::ptr add_player() {
				PlayerODE::ptr	 p(new PlayerODE(eworld, space, the_ball->ballGeom, static_cast<double>(UPDATES_PER_TICK)));
				Point cur =p->position();
				
				Point balpos = the_ball->position();
				Point c = balpos-cur;
				if(c.len()<0.101){
					cur.x+=0.1; 
				}
				
				for (unsigned int i = 0; i < the_players.size(); i++) {
					Point b = the_players[i]->position();
					c = cur-b;
					if(c.len()<0.15){
					cur.x+=0.2;
					}
				}
				
				p->position(cur);
				p->velocity(Point());
				the_players.push_back(p);
				return p;
			}
			
			
			PlayerODE::ptr get_player_from_shape(dGeomID shape){
				for (unsigned int i = 0; i < the_players.size(); i++) {
					if (the_players[i]->robot_contains_shape(shape)) {
						return the_players[i];
					}
				}
				return emptyPlayer;
			}

			
			PlayerODE::ptr get_player_from_shape_ground(dGeomID shape){
				for (unsigned int i = 0; i < the_players.size(); i++) {
					if (the_players[i]->robot_contains_shape_ground(shape)) {
						return the_players[i];
					}
				}
				return emptyPlayer;
			}
			
			
			void remove_player(SimulatorPlayer::ptr p) {
				for (unsigned int i = 0; i < the_players.size(); i++) {
					if (SimulatorPlayer::ptr::cast_static(the_players[i]) == p) {
						the_players.erase(the_players.begin() + i);
						return;
					}
				}
			}

			Gtk::Widget *get_ui_controls() {
				return 0;
			}

			//
			//if a shape interescts with the ground set the contact parameters
			//
			void handleCollisionWithGround(dGeomID o1, dGeomID o2){
				int g1 = (o1 == the_ball->ballGeom);
				int g2 = (o2 == the_ball->ballGeom);
				int i=0;		
				if ((g1 ^ g2)){
					handleBallCollisionWithGround(o1,o2);
				}else if(get_player_from_shape(o1) != emptyPlayer || get_player_from_shape(o2) != emptyPlayer){
				  //make sure that the capped cylinders do not collide with the ground
				  if(get_player_from_shape_ground(o1) != emptyPlayer || get_player_from_shape_ground(o2) != emptyPlayer){
				  dBodyID b1 = dGeomGetBody(o1);
				  dBodyID b2 = dGeomGetBody(o2);

				  dContact contact[3];		// up to 3 contacts per box
				  if (int numc = dCollide (o1,o2,3,&contact[0].geom,sizeof(dContact))) {
				    for (i=0; i<numc; i++) {
				      contact[i].surface.mode = dContactSoftCFM | dContactSoftERP | dContactBounce;
				      contact[i].surface.mu = 0.0;
				      contact[i].surface.soft_cfm = CFM;
				      contact[i].surface.soft_erp = ERP;
				      contact[i].surface.bounce = 0.8;
				      contact[i].surface.bounce_vel =0.0;
				      dJointID c = dJointCreateContact (eworld,contactgroup,contact+i);
				      dJointAttach (c,b1,b2);
				    }
				  }
				  }
				}else{
			
				  dBodyID b1 = dGeomGetBody(o1);
				  dBodyID b2 = dGeomGetBody(o2);

				  dContact contact[3];		// up to 3 contacts per box
				  if (int numc = dCollide (o1,o2,3,&contact[0].geom,sizeof(dContact))) {
				    for (i=0; i<numc; i++) {
				      contact[i].surface.mode = dContactSoftCFM | dContactSoftERP | dContactBounce;
				      contact[i].surface.mu = 0.0;
				      contact[i].surface.soft_cfm = CFM;
				      contact[i].surface.soft_erp = ERP;
				      contact[i].surface.bounce = 0.8;
				      contact[i].surface.bounce_vel =0.0;
				      dJointID c = dJointCreateContact (eworld,contactgroup,contact+i);
				      dJointAttach (c,b1,b2);
				    }
				  }
				
				}

			}
			
			
			//
			//if a shape interescts with the ground set the contact parameters
			//
			void handleBallCollisionWithGround(dGeomID o1, dGeomID o2){

				double frict = MU*6;
				int i=0;
				 PlayerODE::ptr robot = emptyPlayer;
				 
				 for(unsigned int i=0; i<the_players.size(); i++){
					 if(the_players[i]->has_ball()){
					 	robot = the_players[i];
					 }
				 }
				 
				 bool hasBall = robot!=emptyPlayer;
				 

							
				//std::cout<<"ball frict dir "<< (angleVel[0]) <<" "<< (angleVel[1]) <<" "<< (angleVel[2]) <<std::endl;
									
				//if ((g1 ^ g2)){
								
				  dBodyID b1 = dGeomGetBody(o1);
				  dBodyID b2 = dGeomGetBody(o2);

				  dContact contact[3];		// up to 3 contacts per box				  
				  if (int numc = dCollide (o1,o2,3,&contact[0].geom,sizeof(dContact))) {
				  	for (i=0; i<numc; i++) {
				 		contact[i].surface.mode = dContactSoftERP | dContactSoftCFM | dContactApprox1 | dContactBounce;
				 		contact[i].surface.mu = frict;
				   		contact[i].surface.soft_cfm = 0.0;
				   		contact[i].surface.soft_erp = 1.0;
				   		contact[i].surface.bounce = 1.0;
				   		contact[i].surface.bounce_vel = 0.0;
				      		dJointID c = dJointCreateContact (eworld,contactgroup,contact+i);
				      		dJointAttach (c,b1,b2);
				    	}
				  }
				//}
		

			}
			
			//
			//if a shape interescts with the ball set the contact parameters
			//
			void handleBallCollision (dGeomID o1, dGeomID o2){
				  unsigned int i=0;
				  dBodyID b1 = dGeomGetBody(o1);
				  dBodyID b2 = dGeomGetBody(o2);
				  const unsigned int num_contact = 7;
				  
				  dContact contact[num_contact];		// up to 3 contacts per box
				  
				  PlayerODE::ptr robot1 = get_player_from_shape(o1);
				  PlayerODE::ptr robot2 = get_player_from_shape(o2);
				  
				  if((robot1 != emptyPlayer || robot2 != emptyPlayer)){
				  	handleRobotBallCollision(o1,o2);
				  }else{
					if (unsigned int numc = dCollide (o1,o2,num_contact,&contact[0].geom,sizeof(dContact))) {
					    for (i=0; i<numc; i++) {
					  	contact[i].surface.mode = dContactSoftCFM | dContactSoftERP | dContactBounce;
					    	contact[i].surface.mu = MU;
					    	contact[i].surface.soft_cfm = CFM;
					   	contact[i].surface.soft_erp = ERP;
					   	contact[i].surface.bounce = 0.3;
					   	contact[i].surface.bounce_vel=0.0;
					      	dJointID c = dJointCreateContact (eworld,contactgroup,contact+i);
					      	dJointAttach (c,b1,b2);
					    }
					  }	
				  
				  }
			}
			
			//
			//
			//						
			void handleRobotBallCollision (dGeomID o1, dGeomID o2){
				int i=0;
				  dBodyID b1 = dGeomGetBody(o1);
				  dBodyID b2 = dGeomGetBody(o2);
				  dContact contact[3];		// up to 3 contacts per box
				  
				  PlayerODE::ptr robot = get_player_from_shape(o1);
				  if(robot == emptyPlayer){
				  	robot = get_player_from_shape(o2);
				  }
				  if (int numc = dCollide (o1,o2,3,&contact[0].geom,sizeof(dContact))) {

				  	for (i=0; i<numc; i++) {
						bool robotCollided = robot->hasContactPenetration(contact[i].geom.pos);
						bool has_ball = robot->hasContactWithFace(contact[i].geom.pos);
						if(has_ball)robot->set_has_ball();
						if(robotCollided && !robot->has_chip_set() && !robot->has_kick_set()){
				   			contact[i].surface.mode =  dContactSoftCFM | dContactSoftERP |dContactBounce;
				   			contact[i].surface.mu = MU;// 0.1*MU;
				   			contact[i].surface.soft_cfm = CFM;
				   			contact[i].surface.soft_erp = ERP;
				     			contact[i].surface.bounce = 0.2;
				     			contact[i].surface.bounce_vel=0.0;
				      			dJointID c = dJointCreateContact (eworld,contactgroup,contact+i);
				      			dJointAttach (c,b1,b2);
						}

				    	}
				  }	
			}
			
			//
			//if a shape interescts with the wall set the contact parameters
			//robot collisions with the wall are disabled for stability
			//
			void handleWallCollision (dGeomID o1, dGeomID o2){
				int i=0;
				  dBodyID b1 = dGeomGetBody(o1);
				  dBodyID b2 = dGeomGetBody(o2);

				PlayerODE::ptr robot1 = get_player_from_shape(o1);
				PlayerODE::ptr robot2 = get_player_from_shape(o2);

						if(robot1 || robot2){
							return;
						}

				  dContact contact[3];		// up to 3 contacts per box
				  if (int numc = dCollide (o1,o2,3,&contact[0].geom,sizeof(dContact))) {
				  	for (i=0; i<numc; i++) {
				        	contact[i].surface.mode =  dContactSoftCFM| dContactSoftERP | dContactBounce| dContactApprox1;
				    		contact[i].surface.mu = 2.0;
				   		contact[i].surface.soft_cfm = CFM;
				   		contact[i].surface.soft_erp = ERP;
				    		contact[i].surface.bounce = 1.0;
				    		contact[i].surface.bounce_vel = 0.0;
				      		dJointID c = dJointCreateContact (eworld,contactgroup,contact+i);
				      		dJointAttach (c,b1,b2);

				    	}
				  }	
			}
			
			//
			//if ground or ball or wall isn't invloved, we assume a robot robot collision
			//
			void handleRobotRobotCollision (dGeomID o1, dGeomID o2){
				int i=0;
				
				//const dReal* pos1 = dGeomGetPosition (o1);
				//const dReal* pos2 = dGeomGetPosition (o2);
				//Point p1 = Point(pos1[0],pos1[1]);
				//Point p2 = Point(pos2[0],pos2[1]);
				//Point dis = p1-p2;
				
				PlayerODE::ptr robot1 = get_player_from_shape(o1);
				PlayerODE::ptr robot2 = get_player_from_shape(o2);
				
				//if (dis.len>0.02) we assume that are components from same robot
				//as such we will ignore it
				//this is a pretty bad hack and needs to be changed to check whether the geoms come
				//from the same robot
				const int num_contact = 4;
				
				if(robot1!=robot2){
					  dBodyID b1 = dGeomGetBody(o1);
				  	  dBodyID b2 = dGeomGetBody(o2);
					  dContact contact[num_contact];		// up to 3 contacts per box
					  if (int numc = dCollide (o1,o2,num_contact,&contact[0].geom,sizeof(dContact))) {
					    for (i=0; i<numc; i++) {
						bool robotCollided = robot1->hasContactPenetration(contact[i].geom.pos) &&
									robot2->hasContactPenetration(contact[i].geom.pos);
						if(robotCollided){
					    	  contact[i].surface.mode = dContactSoftCFM | dContactSoftERP | dContactBounce;;
					    	  contact[i].surface.mu = MU;
					   	  contact[i].surface.soft_cfm = CFM;
					   	  contact[i].surface.soft_erp = ERP;
					     	  contact[i].surface.bounce = 1.0;
					     	  contact[i].surface.bounce_vel = 0.0;
					      	  dJointID c = dJointCreateContact (eworld,contactgroup,contact+i);
					      	  dJointAttach (c,b1,b2);

						}
					    }
					  }
				}
			}

			//			
			//This gets called every time we have two shpaes in the World that intersect
			//for every pair of intersecting shapes we need to decide what to do with them
			//
			void nearCallback (dGeomID o1, dGeomID o2)
			{
				int groundCollision;
				int notGroundCollision;
				int g1 = (o1 == ground);
				int g2 = (o2 == ground);
				groundCollision = (g1 ^ g2);
				notGroundCollision = !groundCollision;				
				if (groundCollision) {
					handleCollisionWithGround(o1, o2);
				}else if(notGroundCollision) {
					int ballCollision;
					g1 = (o1 == the_ball->ballGeom);
					g2 = (o2 == the_ball->ballGeom);
					ballCollision = (g1 ^ g2);
					
					 bool wall1 = 	 isWall(o1);
					 bool wall2 = 	 isWall(o2);	
					 
					if (ballCollision){
						handleBallCollision(o1, o2);
					}else if(wall1 && wall2){
					//do nothing
					}else if(wall1 || wall2){
						//std::cout<<"wall"<<std::endl;
						handleWallCollision(o1,o2);
					}else{
						handleRobotRobotCollision (o1, o2);
					}
				}
			}
			
			static void nearCallbackThunk(void *data, dGeomID o1, dGeomID o2) {
				SimEngine *engine = reinterpret_cast<SimEngine *>(data);
				engine->nearCallback(o1, o2);
			}
			
			SimulatorEngineFactory &get_factory();


	};

	//
	// A factory for creating sim_engines.
	//
	class SimEngineFactory : public SimulatorEngineFactory {
		public:
			SimEngineFactory() : SimulatorEngineFactory("Open Dynamics Engine Simulator") {
			}

			SimulatorEngine::ptr create_engine() {
				SimulatorEngine::ptr p(new SimEngine);
				return p;
			}
	};

	//
	// The global instance of SimEngineFactory.
	//
	SimEngineFactory fact;

	SimulatorEngineFactory &SimEngine::get_factory() {
		return fact;
	}
}

