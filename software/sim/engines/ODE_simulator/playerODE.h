#include "world/player_impl.h"
#include <ode/ode.h>



/** 
The back-end behind an ODE player object.
*/
class playerODE : public player_impl {
	
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
	
	/// We are mobing the robot
	bool posSet;
	
	///Some vectors for keeping track of the robot
	point the_position, the_velocity, target_velocity, unrotated_target_velocity;
	
	///Some values for robot tracking 
	double the_orientation, avelocity, target_avelocity;
	
	
	/**
	I don't know why we keep track of the ball ID, oh right retarded has ball
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
	
	public:

	playerODE( dWorldID dworld, dSpaceID dspace,  dGeomID ballGeom, double ups_per_tick);
	~playerODE();
	bool has_point(double x, double y) const;
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

			void pre_tic(double TimeStep);
			
			bool has_ball(double tolerance);
			
			void dribble(double speed) ;

			void kick(double strength) ;

			void chip(double strength) ;

			void ext_drag(const point &pos, const point &vel);

			void ext_rotate(double orient, double avel);
			
			void createJointBetweenB1B2();

		 	dTriMeshDataID create_robot_geom();
};



