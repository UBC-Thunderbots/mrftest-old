#include "world/player_impl.h"
#include <ode/ode.h>

//
// The back-end behind an ODE player object.
// 
//
class playerODE : public virtual player_impl {
	public:
//The world constructed by the simulatiuon engine
	dWorldID world;
	playerODE( dWorldID dworld);

void update();

	virtual point position() const ;

			virtual double orientation() const ;

			virtual bool has_ball() const ;
				
			
protected:
			virtual void move_impl(const point &vel, double avel) ;
public:
			virtual void dribble(double speed) ;

			virtual void kick(double strength) ;

			virtual void chip(double strength) ;

			virtual void ui_set_position(const point &pos);

	private:
			point the_position, the_velocity, target_velocity;
			double the_orientation, avelocity;

};



