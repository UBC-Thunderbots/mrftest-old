#ifndef AI_NAVIGATOR_ROBOT_NAVIGATOR_H
#define AI_NAVIGATOR_ROBOT_NAVIGATOR_H

#include "navigator.h"
/**
 * Note about the default behaviour:
 * <ul>
 * <li>Does not have any clipping (all flags are off).</li>
 * <li>Always orient towards the ball.</li>
 * </ul>
 *
 * Thus at every tick:
 * <ul>
 * <li>call set_position to change the position</li>
 * <li>call set_orientation to change orientation</li>
 * <li>call set_flags to set flags.</li>
 * </ul>
 */
class RobotNavigator : public Navigator {
	public:	
		/**
		 * A pointer to a RobotNavigator.
		 */
		typedef RefPtr<RobotNavigator> Ptr;

		/**
		 * Creates a new RobotNavigator.
		 *
		 * \param[in] player the Player to navigate.
		 *
		 * \param[in] world the World in which to navigate.
		 */
		RobotNavigator(Player::Ptr player, World::Ptr world): Navigator(player, world){
		}
		
		//this is kept for legacy purposes
		void tick();	

	private:
	
		Point get_inbounds_point(Point dst);
		Point force_defense_len(Point dst);
		Point force_offense_len(Point dst);
		Point clip_defense_area(Point dst);
		Point clip_offense_area(Point dst);
		bool check_vector(const Point& start, const Point& dest, const Point& direction) const;
		unsigned int check_obstacles(const Point& start, const Point& dest, const Point& direction) const;
		double get_avoidance_factor() const;
		bool check_ball(const Point& start, const Point& dest, const Point& direction) const;
		Point clip_circle(Point circle_centre, double circle_radius, Point dst);
		bool ball_obstacle;
		// clip the field boundries 
		Point clip_playing_area(Point wantdest);

};

class TeamRobotNavigator : protected TeamNavigator{
	protected:
		virtual void tick();
		virtual Navigator::Ptr create_navigator(Player::Ptr play);

};

#endif

