#include "world/player_impl.h"
#include <ode/ode.h>

//
// The back-end behind an ODE player object.
// 
//
class playerODE : public player_impl {
	public:
//The world constructed by the simulatiuon engine
	dWorldID world;
dBodyID body;
dMass mass;
	playerODE( dWorldID dworld, dSpaceID dspace);
//void tick();

void update();

	point position() const ;

			double orientation() const ;

			bool has_ball() const ;
				
			
protected:
			void move_impl(const point &vel, double avel) ;
public:
			void dribble(double speed) ;

			void kick(double strength) ;

			void chip(double strength) ;

			void ui_set_position(const point &pos);

	private:
			point the_position, the_velocity, target_velocity;
			double the_orientation, avelocity;

};



