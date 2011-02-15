#include "geom/angle.h"
#include "simulator/field.h"
#include "simulator/engines/engine.h"
#include "simulator/engines/ODE_simulator/ballODE.h"
#include "simulator/engines/ODE_simulator/playerODE.h"
#include "util/timestep.h"
#include <iostream>

namespace {
	const dReal MU = static_cast<dReal>(0.02);     // the global mu to use

	//
	// The limit of floating-point precision.
	//
	const dReal EPS = static_cast<dReal>(1.0e-9);

	//
	// The force of gravity N/kg
	//
	const dReal GRAVITY = static_cast<dReal>(-9.81);


	const unsigned int UPDATES_PER_TICK = 20;

	//
	//
	//
	const dReal CFM = static_cast<dReal>(1.0e-5);

	//
	//
	//
	const dReal ERP = 1.0;


	//
	// A SimulatorEngine.
	//
	class SimEngine : public SimulatorEngine {
		private:
			BallODE::Ptr the_ball;
			std::vector<PlayerODE::Ptr> the_players;
			PlayerODE::Ptr emptyPlayer;

		public:
			dReal timeStep;
			dWorldID eworld;
			dSpaceID space;
			dGeomID ground;
			dGeomID wall[4];
			dJointGroupID contactgroup;
			// int stepSize;

			bool isWall(dGeomID geom) {
				for (int i = 0; i < 4; i++) {
					if (geom == wall[i]) { return true; }
				}
				return false;
			}

			SimEngine() {
				dInitODE();
				timeStep = static_cast<dReal>(1.0 / (TIMESTEPS_PER_SECOND * UPDATES_PER_TICK));
				eworld = dWorldCreate();
				dWorldSetGravity(eworld, 0, 0.0, GRAVITY);
				space = dSimpleSpaceCreate(0);

				ground = dCreatePlane(space, 0, 0, 1, 0);



				dReal wall_height = static_cast<dReal>(20.5); // 1/2 meter
				dReal wall_thickness = static_cast<dReal>(0.1127); //

				// build a wall around the playing field
				wall[0] = dCreateBox(space, static_cast<dReal>(Simulator::Field::TOTAL_LENGTH) + 2 * wall_thickness, wall_thickness, wall_height);
				wall[1] = dCreateBox(space, static_cast<dReal>(Simulator::Field::TOTAL_LENGTH) + 2 * wall_thickness, wall_thickness, wall_height);
				wall[2] = dCreateBox(space, wall_thickness, static_cast<dReal>(Simulator::Field::TOTAL_WIDTH), wall_height);
				wall[3] = dCreateBox(space, wall_thickness, static_cast<dReal>(Simulator::Field::TOTAL_WIDTH), wall_height);
				dGeomSetPosition(wall[0], 0, (static_cast<dReal>(Simulator::Field::TOTAL_WIDTH) / 2 + wall_thickness / 2), (wall_height / 2));
				dGeomSetPosition(wall[1], 0, -(static_cast<dReal>(Simulator::Field::TOTAL_WIDTH) / 2 + wall_thickness / 2), (wall_height / 2));
				dGeomSetPosition(wall[2], (static_cast<dReal>(Simulator::Field::TOTAL_LENGTH) / 2 + wall_thickness / 2), 0, (wall_height / 2));
				dGeomSetPosition(wall[3], -(static_cast<dReal>(Simulator::Field::TOTAL_LENGTH) / 2 + wall_thickness / 2), 0, (wall_height / 2));
				// set possible penetration for collisions

				dWorldSetContactSurfaceLayer(eworld, static_cast<dReal>(0.1));
				contactgroup = dJointGroupCreate(0);

				BallODE::Ptr b(new BallODE(eworld, space));
				the_ball = b;

				// dWorldSetLinearDamping (eworld, 0.02);
				dWorldSetCFM(eworld, CFM);
			}

			~SimEngine() {
				the_players.clear();
				the_ball.reset();
				dWorldDestroy(eworld);
				dJointGroupDestroy(contactgroup);
				dCloseODE();
			}


			void tick() {
				for (unsigned int i = 0; i < UPDATES_PER_TICK; i++) {
					for (std::size_t j = 0; j < the_players.size(); j++) {
						the_players[j]->pre_tic(timeStep);
					}

					// check the World for possible collisions
					// if there are colliding objects then call nearCallback
					// nearCallback creates all necessary contact points and parameters
					dSpaceCollide(space, this, &SimEngine::nearCallbackThunk);

					// step the World (have ODE do 1 iterations per step)
					dWorldSetQuickStepNumIterations(eworld, 50);
					dWorldQuickStep(eworld, timeStep);

					// remove all the contact points that we created in this step
					dJointGroupEmpty(contactgroup);
				}
			}
			void setWorld(dWorldID world) {
				eworld = world;
			}
			Simulator::Ball::Ptr get_ball() {
				return the_ball;
			}

			Simulator::Player::Ptr add_player() {
				PlayerODE::Ptr p(new PlayerODE(eworld, space, the_ball->ballGeom, UPDATES_PER_TICK));
				Point cur = p->position();

				Point balpos = the_ball->position();
				Point c = balpos - cur;
				if (c.len() < 0.101) {
					cur.x += 0.1;
				}

				for (std::size_t i = 0; i < the_players.size(); i++) {
					Point b = the_players[i]->position();
					c = cur - b;
					if (c.len() < 0.15) {
						cur.x += 0.2;
					}
				}

				p->position(cur);
				p->velocity(Point());
				the_players.push_back(p);
				return p;
			}


			PlayerODE::Ptr get_player_from_shape(dGeomID shape) {
				for (std::size_t i = 0; i < the_players.size(); i++) {
					if (the_players[i]->robot_contains_shape(shape)) {
						return the_players[i];
					}
				}
				return emptyPlayer;
			}

			void remove_player(Simulator::Player::Ptr p) {
				for (std::size_t i = 0; i < the_players.size(); i++) {
					if (Simulator::Player::Ptr::cast_static(the_players[i]) == p) {
						the_players.erase(the_players.begin() + i);
						return;
					}
				}
			}

			Gtk::Widget *get_ui_controls() {
				return 0;
			}

			//
			// if a shape interescts with the ground set the contact parameters
			//
			void handleBallCollisionWithGround(dGeomID o1, dGeomID o2) {
				dReal frict = MU * 12;
				int i = 0;
				PlayerODE::Ptr robot = emptyPlayer;

				for (std::size_t i = 0; i < the_players.size(); i++) {
					if (the_players[i]->has_ball()) {
						robot = the_players[i];
					}
				}

				dBodyID b1 = dGeomGetBody(o1);
				dBodyID b2 = dGeomGetBody(o2);

				dContact contact[3];      // up to 3 contacts per box
				if (int numc = dCollide(o1, o2, 3, &contact[0].geom, sizeof(dContact))) {
					for (i = 0; i < numc; i++) {
						contact[i].surface.mode = dContactSoftERP | dContactSoftCFM | dContactApprox1 | dContactBounce;
						contact[i].surface.mu = frict;
						contact[i].surface.soft_cfm = 0.0;
						contact[i].surface.soft_erp = 1.0;
						// estimate restitution at 0.7
						contact[i].surface.bounce = static_cast<dReal>(0.7);
						contact[i].surface.bounce_vel = 0.0;
						dJointID c = dJointCreateContact(eworld, contactgroup, contact + i);
						dJointAttach(c, b1, b2);
					}
				}
			}

			//
			// if a shape interescts with the ball set the contact parameters
			//
			void handleBallCollision(dGeomID o1, dGeomID o2) {
				unsigned int i = 0;
				dBodyID b1 = dGeomGetBody(o1);
				dBodyID b2 = dGeomGetBody(o2);
				const unsigned int num_contact = 7;

				dContact contact[num_contact];        // up to 3 contacts per box

				PlayerODE::Ptr robot1 = get_player_from_shape(o1);
				PlayerODE::Ptr robot2 = get_player_from_shape(o2);

				if ((robot1 != emptyPlayer || robot2 != emptyPlayer)) {
					std::cout << "  hello world!!!  " << std::endl;
					// handleRobotBallCollision(o1, o2);
				} else {
					if (unsigned int numc = dCollide(o1, o2, num_contact, &contact[0].geom, sizeof(dContact))) {
						for (i = 0; i < numc; i++) {
							contact[i].surface.mode = dContactSoftCFM | dContactSoftERP | dContactBounce;
							contact[i].surface.mu = MU;
							contact[i].surface.soft_cfm = CFM;
							contact[i].surface.soft_erp = ERP;
							contact[i].surface.bounce = static_cast<dReal>(0.3);
							contact[i].surface.bounce_vel = 0.0;
							dJointID c = dJointCreateContact(eworld, contactgroup, contact + i);
							dJointAttach(c, b1, b2);
						}
					}
				}
			}

			//
			// if a shape interescts with the wall set the contact parameters
			// robot collisions with the wall are disabled for stability
			//
			void handleWallCollision(dGeomID o1, dGeomID o2) {
				int i = 0;
				dBodyID b1 = dGeomGetBody(o1);
				dBodyID b2 = dGeomGetBody(o2);

				dContact contact[3];      // up to 3 contacts per box
				if (int numc = dCollide(o1, o2, 3, &contact[0].geom, sizeof(dContact))) {
					for (i = 0; i < numc; i++) {
						contact[i].surface.mode = dContactSoftCFM | dContactSoftERP | dContactBounce | dContactApprox1;
						contact[i].surface.mu = 2.0;
						contact[i].surface.soft_cfm = CFM;
						contact[i].surface.soft_erp = ERP;
						contact[i].surface.bounce = 1.0;
						contact[i].surface.bounce_vel = 0.0;
						dJointID c = dJointCreateContact(eworld, contactgroup, contact + i);
						dJointAttach(c, b1, b2);
					}
				}
			}

			//
			// This gets called every time we have two shpaes in the World that intersect
			// for every pair of intersecting shapes we need to decide what to do with them
			//
			void nearCallback(dGeomID o1, dGeomID o2) {
				int groundCollision;
				int notGroundCollision;
				int g1 = (o1 == ground);
				int g2 = (o2 == ground);
				groundCollision = (g1 ^ g2);
				notGroundCollision = !groundCollision;

				PlayerODE::Ptr robot1 = get_player_from_shape(o1);
				PlayerODE::Ptr robot2 = get_player_from_shape(o2);

				if (robot1 != emptyPlayer) {
					robot1->p_geom.handle_collision(o1, o2, contactgroup);
				} else if (robot2 != emptyPlayer) {
					robot2->p_geom.handle_collision(o1, o2, contactgroup);
				}

				if (robot1 != emptyPlayer || robot2 != emptyPlayer) {
					return;
				}

				if (groundCollision) {
					handleBallCollisionWithGround(o1, o2);
				} else if (notGroundCollision) {
					int ballCollision;
					g1 = (o1 == the_ball->ballGeom);
					g2 = (o2 == the_ball->ballGeom);
					ballCollision = (g1 ^ g2);

					bool wall1 = isWall(o1);
					bool wall2 = isWall(o2);

					if (ballCollision) {
						handleBallCollision(o1, o2);
					} else if (wall1 && wall2) {
						// do nothing
					} else if (wall1 || wall2) {
						handleWallCollision(o1, o2);
					}
				}
			}

			static void nearCallbackThunk(void *data, dGeomID o1, dGeomID o2) {
				SimEngine *engine = static_cast<SimEngine *>(data);
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

			SimulatorEngine::Ptr create_engine() {
				SimulatorEngine::Ptr p(new SimEngine);
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

