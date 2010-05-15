#ifndef AI_NAVIGATOR_ROBOT_NAVIGATOR_H
#define AI_NAVIGATOR_ROBOT_NAVIGATOR_H

#include "ai/navigator.h"

#warning "bloat"

class robot_navigator : public navigator {
	public:
		typedef Glib::RefPtr<robot_navigator> ptr;
		robot_navigator(player::ptr player, field::ptr field, ball::ptr ball, team::ptr team);

		void tick();
		void set_point(const point& destination);

		//set the amount of avoidance 
		void set_slow_avoidance_factor(double factor){
			#warning "hardware dependent parameter"
			slow_avoidance_factor=factor;
		}

		//get the amount of avoidance 
		double get_slow_avoidance_factor() const {
			#warning "hardware dependent parameter"
			return slow_avoidance_factor;
		}

		//set the amount of avoidance 
		void set_fast_avoidance_factor(double factor){
			#warning "hardware dependent parameter"
			fast_avoidance_factor = factor;
		}

		//get the amount of avoidance 
		double get_fast_avoidance_factor() const {
			#warning "hardware dependent parameter"
			return fast_avoidance_factor;
		}

		//get the agression factor that is associateds with the given speed
		double get_avoidance_factor() const;

		void set_correction_step_size(double correction_size);  
		void set_desired_robot_orientation(double orientation);

		void set_robot_avoid_ball_amount(int amount);   
		bool robot_avoids_ball();
		void set_robot_avoids_ball(bool avoid);

		bool robot_avoids_goal();
		void set_robot_avoids_goal(bool avoid);

		bool robot_stays_on_own_half();
		void set_robot_stays_on_own_half(bool avoid);

		void set_robot_avoid_opponent_goal_amount(double amount);
		bool robot_stays_away_from_opponent_goal();
		void set_robot_stays_away_from_opponent_goal(bool avoid);

		static point clip_point(const point& p, const point& bound1, const point& bound2);

	private:
		bool check_vector(const point& start, const point& dest, const point& direction) const;

		bool dest_initialized;//has a destination been specified?
		point curr_dest;//current destination
		double outofbounds_margin;//distance to remain from sidelines to prevent from going oob
		double avoidance_factor; //smaller factor makes robot more aggressive (i.e. less eager to avoid obstacles)
		double slow_avoidance_factor;
		double fast_avoidance_factor; 
		bool avoid_ball;
		bool avoids_goal;
		double avoid_goal_amount;
		bool robot_stays_on_half;
		bool robot_avoids_opponent_goal;
		double avoid_opponent_goal_amount;

		/// Amount of rotation per step to check for collision.
		double rotation_angle;

		/// Amount of rotation before robot gives up checking collision.
		double rotation_thresh;

		/// Max distance of lookahead distance.
		double lookahead_max;

		/// distance of each of lookahead step.
		double lookahead_step;
};

#endif

