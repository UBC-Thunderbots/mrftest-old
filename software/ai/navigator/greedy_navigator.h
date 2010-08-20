#ifndef AI_NAVIGATOR_ROBOT_NAVIGATOR_H
#define AI_NAVIGATOR_ROBOT_NAVIGATOR_H

#include "navigator.h"

#warning this class needs Doxygen comments
/**
 * This is a port of the 2010 navigator into the 2010-2011 framework
 * the navigator handles all players individually, greadily choosing a direction towards
 * the target while avoiding obstacles
 * 
 * the greedy choice sometimes fails, since a player may need to back up to go around said obstacle,
 * it also fails to give the controller any detail in intended path, which could result in optimization
 * 
 */
class TeamGreedyNavigator : protected TeamNavigator{
	protected:
		/**
		 * executes one tick for the entire team
		 */
		virtual void tick();
	private:
		bool ball_obstacle;
	
		/**
		 * this chooses the next direction for the given player
		 *\param[in] play the Player to navigate.
		 */
		void tick(Player::Ptr play);
		/**
		 * This function clips the destination to the allowed playing area
		 *
		 *\param[in] Point dst
		 */	
		Point get_inbounds_point(Point dst);
		
		/**
		 * This function clips the destination to the allowed playing area
		 *
		 *\param[in] Point dst
		 */
		Point force_defense_len(Point dst);
		
		/**
		 * This function clips the destination to the allowed playing area
		 *
		 *\param[in] Point dst
		 */
		Point force_offense_len(Point dst);
		
		/**
		 * This function clips the destination to the allowed playing area
		 *
		 *\param[in] Point dst
		 */		
		Point clip_defense_area(Point dst);
		
		/**
		 * This function clips the destination to the allowed playing area
		 *
		 *\param[in] Point dst
		 */
		Point clip_offense_area(Point dst);
		
		/**
		 * This function checks to see if the given direction is free of obstacles
		 */		
		bool check_vector(const Point& start, const Point& dest, const Point& direction) const;
		
		/**
		 * This function returns a list of obstacles given a certain direction
		 */
		unsigned int check_obstacles(const Point& start, const Point& dest, const Point& direction) const;
		
		/**
		 * returns a number specifying to what degree the robot should avoid abotacles
		 */
		double get_avoidance_factor() const;
		
		/**
		 *checks if the ball is in the way, given a robot's desired path
		 */
		bool check_ball(const Point& start, const Point& dest, const Point& direction) const;
		
		/**
		 * Clips a point so that it does not intersect a given circle
		 */
		Point clip_circle(Point circle_centre, double circle_radius, Point dst);
		

		/**
		 * This function clips the destination to the allowed playing area
		 *
		 *\param[in] Point dst
		 */
		Point clip_playing_area(Point wantdest);
		
		
#warning below is a hack to port old navigator into new framework		
		Player::Ptr the_player;
		std::pair<Point, double> target_position;
		std::pair<double, double> target_orientation;
		unsigned int flags;
		bool need_dribble;
		

};

#endif

