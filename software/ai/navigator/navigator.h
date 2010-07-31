#ifndef AI_NAVIGATOR_NAVIGATOR_H
#define AI_NAVIGATOR_NAVIGATOR_H

#include "ai/world/player.h"
#include "ai/world/robot.h"
#include "ai/world/world.h"
#include "geom/point.h"
#include "util/noncopyable.h"
#include <utility>
#include <map>


/**
 * A player's view of navigation
 * 
 * The TeamNavigator has all the copies of individual  
 * navigators there is a one-to-one relation between 
 * players and Navigators
 */
class Navigator : public ByRef {

	public:
		/**
		 * A pointer to a Navigator object.
		 */
		typedef RefPtr<Navigator> Ptr;	
	
		Navigator(Player::Ptr player, World::Ptr world);

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


/**
 * Single instance of TeamNavigator handles navigation for all players on the controllable team
 */
class TeamNavigator : public NonCopyable, public sigc::trackable{
	public:
		TeamNavigator(World::Ptr world):the_world(world){
			the_world->friendly.signal_player_removed.connect(sigc::mem_fun(this, &TeamNavigator::on_player_removed));
			the_world->friendly.signal_player_added.connect(sigc::mem_fun(this, &TeamNavigator::on_player_added));
			the_world->enemy.signal_robot_removed.connect(sigc::mem_fun(this, &TeamNavigator::on_enemy_removed));
			the_world->enemy.signal_robot_added.connect(sigc::mem_fun(this, &TeamNavigator::on_enemy_added));
		}
		
		/**
		* map from player to the player's navigator
		* the rest of the ai operates on the player's navigator, not the team navigator
		*/
		std::map<Player::Ptr, Navigator::Ptr> navis;
		
	private:

	
		void tick();
		
		/**
		* When a player is added will need to make a new player navigator for it
		*/
		void on_player_added(unsigned int, Player::Ptr play);
		
		/**
		* When a player is removed will need to destroy player navigator for it
		*/		
		void on_player_removed(unsigned int, Player::Ptr play);
		
		/**
		* We may want to keep tabs on the enemy
		*/
		void on_enemy_added(unsigned int, Robot::Ptr bot);
		
		/**
		* We may want to keep tabs on the enemy
		*/		
		void on_enemy_removed(unsigned int, Robot::Ptr bot);
	
		const World::Ptr the_world;

};

#endif

