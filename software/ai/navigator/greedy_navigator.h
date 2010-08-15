#ifndef AI_NAVIGATOR_ROBOT_NAVIGATOR_H
#define AI_NAVIGATOR_ROBOT_NAVIGATOR_H

#include "navigator.h"


class TeamGreedyNavigator : protected TeamNavigator{
	protected:
		virtual void tick();
	private:
		void tick(Player::Ptr play);	
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
		
		
#warning below is a hack to port old navigator into new framework		
		Player::Ptr the_player;
		std::pair<Point, double> target_position;
		std::pair<double, double> target_orientation;
		unsigned int flags;
		bool need_dribble;
		

};

#endif

