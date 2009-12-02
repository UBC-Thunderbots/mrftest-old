#include "simulator/engine.h"
#include "simulator/engines/ballODE.h"
#include "simulator/engines/playerODE.h"
#include "simulator/field.h"
#include "world/timestep.h"
#include "geom/angle.h"
#include <iostream>

#define MU 0.02		// the global mu to use
namespace {

	//
	// The limit of floating-point precision.
	//
	const double EPS = 1.0e-9;

	//
	// The maximum acceleration of a robot, in metres per second squared.
	//
	const double BOT_MAX_ACCELERATION = 10.0;

	//
	// The maximum velocity of a robot, in metres per second.
	//
	const double BOT_MAX_VELOCITY = 5.0;

	//
	// The acceleration due to friction against the ball, in metres per second squared.
	//
	const double BALL_DECELERATION = 6.0;

	//
	// The force of gravity N/kg
	//
	const double GRAVITY = -0.25;
	
	
	
	const unsigned int UPDATES_PER_TICK = 1;

	//
	// A simulator_engine.
	//
	class sim_engine : public simulator_engine {
		private:
			ballODE::ptr the_ball;
			std::vector<playerODE::ptr> the_players;
			playerODE::ptr emptyPlayer;
		public:
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

			sim_engine(){
				dInitODE();
				eworld = dWorldCreate(); 
				dWorldSetGravity (eworld,0,0.0,GRAVITY);
				space = dSimpleSpaceCreate(0);
				//space = dHashSpaceCreate(0);
  				ground = dCreatePlane (space,0,0,1,0.1);
  				const field::ptr fld(new simulator_field);
  				
  				double wall_height = 20.5;//1/2 meter
  				double wall_thickness = 0.5;//
  				
				//build a wall around the playing field
  				wall[0] = dCreateBox (space, fld->total_length() + 1.0, wall_thickness, wall_height);
				wall[1] = dCreateBox (space, fld->total_length() + 1.0, wall_thickness, wall_height);  				
				wall[2] = dCreateBox (space, wall_thickness, fld->total_width() + 1.0, wall_height);
				wall[3] = dCreateBox (space, wall_thickness, fld->total_width() + 1.0, wall_height);
  				dGeomSetPosition (wall[0],  0,  (fld->total_width()/2 + wall_thickness/2),  (wall_height/2) -0.1);
  				dGeomSetPosition (wall[1],  0, - (fld->total_width()/2 + wall_thickness/2),  (wall_height/2) -0.1);
  				dGeomSetPosition (wall[2],  (fld->total_length()/2 + wall_thickness/2), 0,  (wall_height/2) -0.1);
  				dGeomSetPosition (wall[3], - (fld->total_length()/2 + wall_thickness/2), 0,  (wall_height/2) -0.1);

				//set possible penetration for collisions
    				dWorldSetContactSurfaceLayer(eworld, 0.001);
				contactgroup = dJointGroupCreate (0);

				ballODE::ptr b(new ballODE(eworld, space));
				the_ball = b;
				
 				//dWorldSetLinearDamping (eworld, 0.02);
				dWorldSetCFM (eworld, 0.5);
 				
			}
			
			~sim_engine(){
				the_players.clear();
 				the_ball.reset();
 				dWorldDestroy (eworld);
 				dJointGroupDestroy (contactgroup);
 				dCloseODE();
			}


			void tick() {
					
				for(unsigned int i=0; i< UPDATES_PER_TICK; i++){
				
					if(i>0){
						for (unsigned int j = 0; j < the_players.size(); j++) {
							the_players[j]->pre_tic();

						}
					}
					//std::cout<<"tick Start"<<std::endl;
					//check the world for possible collisions
					//if there are colliding objects then call nearCallback
					//nearCallback creates all necessary contact points and parameters
	 				dSpaceCollide (space,this,&sim_engine::nearCallbackThunk);
	 				//step the world (have ODE do 1 iterations per step)
					//dWorldStep (eworld, 1);
					dWorldSetQuickStepNumIterations (eworld, 50);
					//double timeStep = 1.0/static_cast<double>(UPDATES_PER_TICK);
					double timeStep = 1.0/static_cast<double>(TIMESTEPS_PER_SECOND);
					dWorldQuickStep(eworld, timeStep);
					//remove all the contact points that we created in this step
					dJointGroupEmpty (contactgroup);
					//std::cout<<"tick End"<<std::endl;
				
				}
			}
			void setWorld(dWorldID world) {
				eworld = world;
			}
			simulator_ball_impl::ptr get_ball() {
				return the_ball;
			}

			player_impl::ptr add_player() {
				playerODE::ptr	 p(new playerODE(eworld, space, the_ball->ballGeom, static_cast<double>(UPDATES_PER_TICK)));
				point cur =p->position();
				
				point balpos = the_ball->position();
				point c = balpos-cur;
				if(c.len()<0.101){
					cur.x+=0.1; 
				}
				
				for (unsigned int i = 0; i < the_players.size(); i++) {
					point b = the_players[i]->position();
					c = cur-b;
					if(c.len()<0.15){
					cur.x+=0.2;
					}
				}
				
				p->ext_drag(cur, point());
				the_players.push_back(p);
				return p;
			}
			
			
			playerODE::ptr get_player_from_shape(dGeomID shape){
				for (unsigned int i = 0; i < the_players.size(); i++) {
					if (the_players[i]->robot_contains_shape(shape)) {
						return the_players[i];
					}
				}
				return emptyPlayer;
			}
			
			
			void remove_player(player_impl::ptr p) {
				for (unsigned int i = 0; i < the_players.size(); i++) {
					if (player_impl::ptr::cast_static(the_players[i]) == p) {
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
				}else{
			
				  dBodyID b1 = dGeomGetBody(o1);
				  dBodyID b2 = dGeomGetBody(o2);

				  dContact contact[3];		// up to 3 contacts per box
				  for (i=0; i<3; i++) {
				    contact[i].surface.mode = dContactSoftCFM | dContactApprox1;
				    contact[i].surface.mu = 0.0;
				   contact[i].surface.soft_cfm = 0.5;
				  }
				  if (int numc = dCollide (o1,o2,3,&contact[0].geom,sizeof(dContact))) {
				    for (i=0; i<numc; i++) {
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
				int g1 = (o1 == the_ball->ballGeom);
				int g2 = (o2 == the_ball->ballGeom);
				double frict = MU*6;
				int i=0;
				
				
				
				 playerODE::ptr robot = emptyPlayer;
				 
				 for(unsigned int i=0; i<the_players.size(); i++){
					 if(the_players[i]->has_ball()){
					 	robot = the_players[i];
					 }
				 }
				 
				 bool hasBall = robot!=emptyPlayer;
				 

							
				//std::cout<<"ball frict dir "<< (angleVel[0]) <<" "<< (angleVel[1]) <<" "<< (angleVel[2]) <<std::endl;
									
				if ((g1 ^ g2)){
								
				  dBodyID b1 = dGeomGetBody(o1);
				  dBodyID b2 = dGeomGetBody(o2);

				  dContact contact[3];		// up to 3 contacts per box
				  for (i=0; i<3; i++) {
				  	if(hasBall && false){
				  		contact[i].surface.mode = dContactSoftCFM | dContactApprox1 | dContactFDir1 | dContactSlip1;
				  			point t(1,0);
				  			point dir = t.rotate(robot->orientation());
				  						   
				    contact[i].fdir1[0] = t.x;
				    contact[i].fdir1[1] = t.y;
				    contact[i].fdir1[2] = 0.0;
				      contact[i].surface.mu = MU*6;
				    //  contact[i].surface.mu2 = frict;
				 	}else{
				 		contact[i].surface.mode = dContactSoftCFM | dContactApprox1;
				 		  contact[i].surface.mu = frict;
				 	}
				 	

				  
				   contact[i].surface.soft_cfm = 0.5;
				  }
				  if (int numc = dCollide (o1,o2,3,&contact[0].geom,sizeof(dContact))) {
				    for (i=0; i<numc; i++) {
				      dJointID c = dJointCreateContact (eworld,contactgroup,contact+i);
				      dJointAttach (c,b1,b2);
				    }
				  }
				}
		

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
				  
				  playerODE::ptr robot1 = get_player_from_shape(o1);
				  playerODE::ptr robot2 = get_player_from_shape(o2);
				  
				  if((robot1 != emptyPlayer || robot2 != emptyPlayer)){
				  	handleRobotBallCollision(o1,o2);
				  }else{
				  
					  for (i=0; i<num_contact; i++) {
					    contact[i].surface.mode = dContactSoftCFM;
					    contact[i].surface.mu = MU;
					    //contact[i].surface.mu
					   contact[i].surface.soft_cfm = 0.3;
					  }
					  if (unsigned int numc = dCollide (o1,o2,num_contact,&contact[0].geom,sizeof(dContact))) {
					    for (i=0; i<numc; i++) {
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
				  
				  playerODE::ptr robot = get_player_from_shape(o1);
				  if(robot == emptyPlayer){
				  	robot = get_player_from_shape(o2);
				  }
				  
				//  point direction(0.0, 1.0);
				//  direction = direction.rotate(robot->orientation());
				  //dReal[4]
				  
				  
				//  dReal vec[4];
				//  vec[0] = direction.x;
				//  vec[1] = direction.y;
				//  vec[2] = 0.0;
				//  dVector3 fr1(vec[0], vec[1], vec[2]);
				  
				  for (i=0; i<3; i++) {
				  //  contact[i].surface.mode =  dContactMu2 |dContactMotion1 | dContactFDir1 |dContactSoftCFM | dContactApprox1;
				  contact[i].surface.mode =  dContactSoftCFM |dContactBounce;
				   // contact[i].fdir1[0] = vec[0];
				    //contact[i].fdir1[1] = vec[1];
				   // contact[i].fdir1[2] = vec[2];
				    contact[i].surface.mu = MU;// 0.1*MU;
				   // contact[i].surface.mu2 = 0.1*MU;
				   contact[i].surface.soft_cfm = 0.5;
				     contact[i].surface.bounce = 0.2;
				  }
				  if (int numc = dCollide (o1,o2,3,&contact[0].geom,sizeof(dContact))) {
				    for (i=0; i<numc; i++) {
				      dJointID c = dJointCreateContact (eworld,contactgroup,contact+i);
				      dJointAttach (c,b1,b2);
				    }
				  }	
			}
			
			//
			//if a shape interescts with the wall set the contact parameters
			//
			void handleWallCollision (dGeomID o1, dGeomID o2){
				int i=0;
				  dBodyID b1 = dGeomGetBody(o1);
				  dBodyID b2 = dGeomGetBody(o2);
				  dContact contact[3];		// up to 3 contacts per box
				  for (i=0; i<3; i++) {
				    contact[i].surface.mode =  dContactSoftCFM | dContactBounce| dContactApprox1;
				    contact[i].surface.mu = 2.0;
				   contact[i].surface.soft_cfm = 0.5;
				    contact[i].surface.bounce = 1.0;
				  }
				  if (int numc = dCollide (o1,o2,3,&contact[0].geom,sizeof(dContact))) {
				    for (i=0; i<numc; i++) {
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
				//point p1 = point(pos1[0],pos1[1]);
				//point p2 = point(pos2[0],pos2[1]);
				//point dis = p1-p2;
				
				playerODE::ptr robot1 = get_player_from_shape(o1);
				playerODE::ptr robot2 = get_player_from_shape(o2);
				
				//if (dis.len>0.02) we assume that are components from same robot
				//as such we will ignore it
				//this is a pretty bad hack and needs to be changed to check whether the geoms come
				//from the same robot
				const int num_contact = 12;
				
				if(robot1!=robot2){
					  dBodyID b1 = dGeomGetBody(o1);
				  	  dBodyID b2 = dGeomGetBody(o2);
					  dContact contact[num_contact];		// up to 3 contacts per box
					  for (i=0; i<num_contact; i++) {
					    contact[i].surface.mode = dContactSoftCFM | dContactApprox1 |dContactBounce;;
					    contact[i].surface.mu = MU;
					   contact[i].surface.soft_cfm = 0.5;
					     contact[i].surface.bounce = 1.0;
					  }
					  if (int numc = dCollide (o1,o2,num_contact,&contact[0].geom,sizeof(dContact))) {
					    for (i=0; i<numc; i++) {
					      dJointID c = dJointCreateContact (eworld,contactgroup,contact+i);
					      dJointAttach (c,b1,b2);
					    }
					  }
				}
			}

			//			
			//This gets called every time we have two shpaes in the world that intersect
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
				sim_engine *engine = reinterpret_cast<sim_engine *>(data);
				engine->nearCallback(o1, o2);
			}
			
			simulator_engine_factory &get_factory();


	};

	//
	// A factory for creating sim_engines.
	//
	class sim_engine_factory : public simulator_engine_factory {
		public:
			sim_engine_factory() : simulator_engine_factory("Open Dynamics Engine Simulator") {
			}

			simulator_engine::ptr create_engine(xmlpp::Element *) {
				simulator_engine::ptr p(new sim_engine);
				return p;
			}
	};

	//
	// The global instance of sim_engine_factory.
	//
	sim_engine_factory fact;

	simulator_engine_factory &sim_engine::get_factory() {
		return fact;
	}
}

