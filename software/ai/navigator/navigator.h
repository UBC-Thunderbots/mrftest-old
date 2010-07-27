#ifndef AI_NAVIGATOR_NAVIGATOR_H
#define AI_NAVIGATOR_NAVIGATOR_H

#include "ai/world/player.h"
#include "ai/world/world.h"
#include "geom/point.h"
#include "util/noncopyable.h"
#include <utility>

/**
 * Single instance of TeamNavigator handles navigation for all players on the controllable team
 */
class TeamNavigator : public NonCopyable {
	public:
		Navigator(World::Ptr world);
		
		/**
		* This function is intended to be called really early in the ai
		* this will be check the team and clear out all robots with no signals
		* so that pointers can be cleared and there are no leaks
		*/
		pre_tic();
		void tick();
	private:
	
};

/**
 * A player's view of navigation
 * 
 * The TeamNavigator has all the copies of individual  
 * navigators there is a one-to-one relation between 
 * players and Navigators
 */
class Navigator : public NonCopyable {

	public:
		Navigator(Player::Ptr player, World::Ptr world);
		~Navigator();
		/**
		 * Sets the desired location.
		 * the error parameter defines a circle where the robot is "close enough" to 
		 * it's target
		 */
		void set_position(const Point& position, double error) {
			target_position.first = position;
			target_position.second = error;
		}

		/**
		 * Normally the navigator sets the robot orientation to be towards the ball.
		 * Use this if you want to override this behaviour.
		 * This only sets the desired orientation for one timestep.
		 * You have to call this function every timestep.
		 * \param orientation
		 */
		void set_orientation(const double& orientation, double error) {
			orientation_initialized = true;
			target_orientation.first = orientation;
			target_orientation.second = error;
		}

		/**
		 * Turns on dribbler at minimal speed and be ready to dribble to Receive the ball.
		 * You need to call this every tick.
		 * I don't think you ever want to turn this off once you turn it on.
		 */
		void set_dribbler(bool dribble) {
			need_dribble = dribble;
		}

		/**
		 * Sets flags 
		 */
		void set_flags(unsigned int f) {
			flags |= f;
		}
		
		/**
		 * un-sets flags
		 */
		void unset_flags(unsigned int f) {
			flags &= ~f;
		}
	protected:
		/**
		*specifies a target position and an error
		*/ 
		std::pair<Point,double> target_position;
		
		/**
		*specifies a target orientation and an error
		*/
		std::pair<double,double> target_orientation;
		
		/*
		*specifies whether the robot should dribble when it has the ball
		*/
		bool need_dribble;
		
		/*
		*
		*/
		unsigned int flags;
	private:
	
};

#endif

