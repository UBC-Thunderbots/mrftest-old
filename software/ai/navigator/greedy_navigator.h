#ifndef AI_NAVIGATOR_ROBOT_NAVIGATOR_H
#define AI_NAVIGATOR_ROBOT_NAVIGATOR_H

#include "navigator.h"

namespace AI {
	namespace Navigator {
		/**
		 * This is a port of the 2010 navigator into the 2010–2011 framework.
		 * The navigator handles all players individually, greedily choosing a direction towards the target while avoiding obstacles.
		 * 
		 * The greedy choice sometimes fails, since a player may need to back up to go around said obstacle;
		 * it also fails to give the controller any detail in intended path, which could result in optimization.
		 */
		class TeamGreedyNavigator : protected TeamNavigator{
			protected:
				/**
				 * Executes one tick for the entire team.
				 */
				virtual void tick();
			private:
				bool ball_obstacle;

				/**
				 * Chooses the next direction for the given player.
				 *
				 * \param[in] play the Player to navigate.
				 */
				void tick(Player::Ptr play);
				/**
				 * Clips the destination to the allowed playing area.
				 *
				 * \param[in] dst the destination to clip.
				 *
				 * \return the clipped destination.
				 */	
				Point get_inbounds_point(Point dst);

				/**
				 * Clips the destination to the allowed playing area.
				 *
				 * \param[in] dst the destination to clip.
				 *
				 * \return the clipped destination
				 */
				Point force_defense_len(Point dst);

				/**
				 * Clips the destination to the allowed playing area.
				 *
				 * \param[in] dst the destination to clip.
				 *
				 * \return the clipped destination.
				 */
				Point force_offense_len(Point dst);

				/**
				 * Clips the destination to the allowed playing area.
				 *
				 * \param[in] dst the destination to clip.
				 *
				 * \return the clipped destination.
				 */		
				Point clip_defense_area(Point dst);

				/**
				 * Clips the destination to the allowed playing area.
				 *
				 * \param[in] dst the destination to clip.
				 *
				 * \return the clipped destination.
				 */
				Point clip_offense_area(Point dst);

				/**
				 * Checks to see if the given direction is free of obstacles.
				 */
#warning document parameters and return value
				bool check_vector(const Point& start, const Point& dest, const Point& direction) const;

				/**
				 * This function returns a list of obstacles given a certain direction
				 */
#warning document parameters and return value
				unsigned int check_obstacles(const Point& start, const Point& dest, const Point& direction) const;

				/**
				 * Returns a number specifying to what degree the robot should avoid obstacles.
				 *
				 * \return the avoidance factor.
				 */
				double get_avoidance_factor() const;

				/**
				 * Checks if the ball is in the way, given a robot's desired path.
				 */
#warning document parameters and return value
				bool check_ball(const Point& start, const Point& dest, const Point& direction) const;

				/**
				 * Clips a point so that it does not intersect a given circle.
				 *
				 * \param[in] circle_centre the centre of the circle.
				 *
				 * \param[in] circle_radius the radius of the circle.
				 *
				 * \param[in] dst the point to clip.
				 *
				 * \return the nearest point to \p dst that does not lie within the circle.
				 */
				Point clip_circle(Point circle_centre, double circle_radius, Point dst);


				/**
				 * Clips the destination to the allowed playing area.
				 *
				 * \param[in] wantdest the destination to clip.
				 *
				 * \return the clipped destination.
				 */
				Point clip_playing_area(Point wantdest);


#warning below is a hack to port old navigator into new framework		
				Player::Ptr the_player;
				std::pair<Point, double> target_position;
				std::pair<double, double> target_orientation;
				unsigned int flags;
				bool need_dribble;


		};
	}
}

#endif

