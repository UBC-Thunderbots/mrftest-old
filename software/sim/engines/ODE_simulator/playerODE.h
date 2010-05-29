#include "sim/player.h"
#include "xbee/shared/packettypes.h"
#include <ode/ode.h>



/** 
The back-end behind an ODE player object.
*/
class playerODE : public player {
	
	public:
		double x_len;
		double y_len;
		double momentInertia;
	

	
	typedef Glib::RefPtr<playerODE> ptr;
	
	private:
	///The rectangular geometry for the front collision
	dGeomID robotGeomTop;
	
	///The Cylindrical geometry for most collisions
	dGeomID robotGeomTopCyl;
	
	///not used
	dGeomID dribbleArmL;
	
	///not used
	dGeomID dribbleArmR;
	
	///we need to interact with the simulator world so store its ID here
	dWorldID world;
	
	///The ID for the robots body in the simulator
	dBodyID body;
	
	///The mass object for the robot, keeps track of things like inertial moments
	dMass mass;
	
	/// We are moving the robot
	bool posSet;
	
	///Some vectors for keeping track of the robot
	point target_velocity, unrotated_target_velocity;
	
	
	bool player_has_ball;
	
	bool chip_set,kick_set;	
	
	double chip_strength,kick_strength;
	
	/**
	I don't know why we keep track of the ball ID, oh right the retarded hasball
	routine Number 1
	*/
	dGeomID ballGeom;
	
	
	double updates_per_tick;
	double jerkLimit;
	double fcex,fcey,torquez;
	dVector3 *Vertices;
	unsigned int *Triangles;
	point* wheel_position;
	point* force_direction;
	
	///Target wheel velocities in quarter of a degree per 5 milliseconds
	double motor_desired[4];

	///Should the robot run in direct drive mode
	bool direct_drive;

	///Should the robot run in controlled drive mode
	bool controlled_drive;


	public:

	playerODE( dWorldID dworld, dSpaceID dspace,  dGeomID ballGeom, double ups_per_tick);
	~playerODE();

	double get_height() const;
			
	void update();

	point position() const ;

	double orientation() const ;

	bool has_ball() const ;
			
			
	bool robot_contains_shape(dGeomID geom);
	bool robot_contains_shape_ground(dGeomID geom);
			
protected:

			
			/**
				method for the AI to control the robots movement
				\param vel desired robot velocity for control
				\param avel desired robot angular velocity
			*/
			void move_impl(const point &vel, double avel) ;
			
public:
			bool hasContactPenetration(dVector3 pos); 
			
			bool hasContactWithFace(dVector3 pos);
			/**
			call this when we find a robot ball collision may do some additional testing beyond this to make sure "has ball"
			*/
			void set_has_ball();

			void pre_tic(double TimeStep);
			
			void dribble(double speed) ;

			void kick(double strength) ;
			
			bool has_kick_set(){
				return kick_set;
			}
			
private:
			bool execute_kick() ;
public:
			void chip(double strength) ;
			
			bool has_chip_set(){
				return chip_set;
			}
private:			
			bool execute_chip() ;
public:
			void position(const point &pos);

			void velocity(const point &vel);

			void orientation(double orient);

			void avelocity(double avel);

			void received(const xbeepacket::RUN_DATA &);
			
			void createJointBetweenB1B2();

		 	dTriMeshDataID create_robot_geom();
};



