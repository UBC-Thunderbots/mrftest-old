#ifndef AI_TACTIC_pivot_H
#define AI_TACTIC_pivot_H

#include "ai/world/world.h"
#include "ai/tactic/tactic.h"
#include "geom/point.h"

/**
 * Chases after the ball,
 * and aims by doing a pivoting turn towards the specified target
 * Then chases the target with the ball
 */
class Pivot : public Tactic {
	public:
		//
		// A pointer to this Tactic.
		//
		typedef Glib::RefPtr<Pivot> ptr;

		/**
		 * Set a target that robot would like to aim the ball
		 * after gaining possesion.
		 */
		void set_target(const Point& t) {
			target = t;
		}

		/**
		* Constructs a new Pivot Tactic. 
		*/
		Pivot(Player::ptr player, World::ptr world);

		//
		// Runs the AI for one time tick.
		//
		void tick();	

		/**
		 * Enables a player to pivot without approaching the ball.
		 */
		void avoid_ball(const bool& b = true) {
			avoid_ball_ = b;
		}

		/**
		 * Calc pivot position.
		 * Useful for roles to know if robot is aligned correctly.
		 */
		static Point calc_pivot_pos(const Point& ballpos, const Point& target);

	protected:

		void tick_experimental();	
		void tick_old();

		const World::ptr the_world;
		Point target;
		bool avoid_ball_;
};

#endif

