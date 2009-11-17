#include "simulator/engine.h"
#include "simulator/engines/ballODE.h"
#include "simulator/engines/playerODE.h"
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

	//
	// A simulator_engine.
	//
	class sim_engine : public simulator_engine {
		private:
			ballODE::ptr the_ball;
			std::vector<playerODE::ptr> the_players;
		public:
			dWorldID eworld;
			dSpaceID space;
			dGeomID ground;
			dJointGroupID contactgroup;

			sim_engine(){
				dInitODE();
				eworld = dWorldCreate(); 
				dWorldSetGravity (eworld,0,0.0,GRAVITY);
				space = dSimpleSpaceCreate(0);
  				ground = dCreatePlane (space,0,0,1,0.1);
    				dWorldSetContactSurfaceLayer(eworld, 0.05);
				contactgroup = dJointGroupCreate (0);

				ballODE::ptr b(new ballODE(eworld, space));
				the_ball = b;
				

 				dWorldSetLinearDamping (eworld, 0.02);
				dWorldSetCFM (eworld, 0.2);
 				
			}
			
			~sim_engine(){
				the_players.clear();
 				the_ball.reset();
 				dWorldDestroy (eworld);
 				 dJointGroupDestroy (contactgroup);

			}


			void tick() {
			
				//check the world for possible collisions
				//if there are colliding objects then call nearCallback
				//nearCallback creates all necessary contact points and parameters
 				dSpaceCollide (space,this,&sim_engine::nearCallbackThunk);
 				
 				//step the world (have ODE do 1 iterations per step)
				dWorldStep (eworld, 1);
				
				//remove all the contact points that we created in this step
				dJointGroupEmpty (contactgroup);
			}
			void setWorld(dWorldID world) {
				eworld = world;
			}
			ball_impl::ptr get_ball() {
				return the_ball;
			}

			playerODE::ptr add_player() {
				playerODE::ptr p(new playerODE(eworld, space));
				point cur =p->position();
				
				point balpos = the_ball->position();
				point c = balpos-cur;
				if(c.len()<0.101){
					cur.x+=0.1; 
				}
				
				for (unsigned int i = 0; i < the_players.size(); i++) {
					point b = playerODE::ptr::cast_static(the_players[i])->position();
					c = cur-b;
					if(c.len()<0.15){
					cur.x+=0.2;
					}
				}
				
				p->ui_set_position(cur);
				the_players.push_back(p);
				return p;
			}

			void remove_player(player_impl::ptr p) {
				for (unsigned int i = 0; i < the_players.size(); i++) {
					if (playerODE::ptr::cast_static(the_players[i]) == p) {
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
		double frict = MU;
		int i=0;		
		if ((g1 ^ g2)){
			frict = MU*60;
		}
		  dBodyID b1 = dGeomGetBody(o1);
		  dBodyID b2 = dGeomGetBody(o2);

		  dContact contact[3];		// up to 3 contacts per box
		  for (i=0; i<3; i++) {
		    contact[i].surface.mode = dContactSoftCFM | dContactApprox1;
		    contact[i].surface.mu = frict;
		    contact[i].surface.soft_cfm = 0.01;
		  }
		  if (int numc = dCollide (o1,o2,3,&contact[0].geom,sizeof(dContact))) {
		    for (i=0; i<numc; i++) {
		      dJointID c = dJointCreateContact (eworld,contactgroup,contact+i);
		      dJointAttach (c,b1,b2);
		    }
		  }
	}
	
	//
	//if a shape interescts with the ball set the contact parameters
	//
	void handleBallCollision (dGeomID o1, dGeomID o2){
		int i=0;
		  dBodyID b1 = dGeomGetBody(o1);
		  dBodyID b2 = dGeomGetBody(o2);
		  dContact contact[3];		// up to 3 contacts per box
		  for (i=0; i<3; i++) {
		    contact[i].surface.mode = dContactSoftCFM | dContactApprox1;
		    contact[i].surface.mu = MU;
		    contact[i].surface.soft_cfm = 0.01;
		  }
		  if (int numc = dCollide (o1,o2,3,&contact[0].geom,sizeof(dContact))) {
		    for (i=0; i<numc; i++) {
		      dJointID c = dJointCreateContact (eworld,contactgroup,contact+i);
		      dJointAttach (c,b1,b2);
		    }
		  }	
	}
	
	//
	//if ground or ball isn't invloved, we assume a robot robot collision
	//
	void handleRobotRobotCollision (dGeomID o1, dGeomID o2){
		int i=0;
		const dReal* pos1 = dGeomGetPosition (o1);
		const dReal* pos2 = dGeomGetPosition (o2);
		point p1 = point(pos1[0],pos1[1]);
		point p2 = point(pos2[0],pos2[1]);
		point dis = p1-p2;
		//if (dis.len>0.02) we assume that are components from same robot
		//as such we will ignore it
		//this is a pretty bad hack and needs to be changed to check whether the geoms come
		//from the same robot
		if(dis.len()>0.02){
			  dBodyID b1 = dGeomGetBody(o1);
		  	  dBodyID b2 = dGeomGetBody(o2);
			  dContact contact[3];		// up to 3 contacts per box
			  for (i=0; i<3; i++) {
			    contact[i].surface.mode = dContactSoftCFM | dContactApprox1;
			    contact[i].surface.mu = MU;
			    contact[i].surface.soft_cfm = 0.01;
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
			if (ballCollision){
				handleBallCollision(o1, o2);
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

