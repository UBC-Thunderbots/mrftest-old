#include "world/player_impl.h"
#include <ode/ode.h>



//
// The back-end behind an ODE player object.
// 
//
class playerODE : public player_impl {
	public:
	
	double x_len;
	double y_len;
	double momentInertia;
	
	typedef Glib::RefPtr<playerODE> ptr;
	//The world constructed by the simulatiuon engine
	private:
	
	dWorldID world;
	dBodyID body;
	dMass mass;
	bool posSet;
	point the_position, the_velocity, target_velocity;
	double the_orientation, avelocity, target_avelocity;
	dGeomID ballGeom;
	double updates_per_tick;
	double jerkLimit;
	double fcex,fcey,torquez;
	public:

	playerODE( dWorldID dworld, dSpaceID dspace,  dGeomID ballGeom, double ups_per_tick);
	~playerODE();
//void tick();

void update();

	point position() const ;

			double orientation() const ;

			bool has_ball() const ;
			
			
		//
		//
		//
		bool robot_contains_shape(dGeomID geom);
				
			
protected:
			void move_impl(const point &vel, double avel) ;
			
public:

			void pre_tic();
			
			bool has_ball(double tolerance);
			
			void dribble(double speed) ;

			void kick(double strength) ;

			void chip(double strength) ;

			void ext_drag(const point &pos, const point &vel);

			void ext_rotate(double orient, double avel);
			
			void createJointBetweenB1B2();

			static dTriMeshDataID CreateRobotGeom();
};



