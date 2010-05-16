#ifndef AI_AI_H
#define AI_AI_H

#include "ai/strategy/strategy.h"
#include "ai/world/world.h"
#include "robot_controller/robot_controller.h"
#include "util/clocksource.h"
#include "util/noncopyable.h"

/**
 * A complete AI.
 */
class ai : public noncopyable {
	public:
		/**
		 * The world in which the AI is running.
		 */
		const world::ptr the_world;

		/**
		 * Constructs a new AI.
		 *
		 * \param world the world to operate in
		 *
		 * \param clk the clock to run the AI from
		 */
		ai(world::ptr world, clocksource &clk);

		/**
		 * \return The strategy driving the robots
		 */
		strategy::ptr get_strategy() const {
			return the_strategy;
		}

		/**
		 * Sets which strategy is driving the robots.
		 *
		 * \param strat the new strategy
		 */
		void set_strategy(strategy::ptr strat);

		/**
		 * \return The robot controller factory driving the robots
		 */
		robot_controller_factory *get_robot_controller_factory() const {
			return the_rc_factory;
		}

		/**
		 * Sets the robot controller factory that creates robot controllers that
		 * drive the robots.
		 *
		 * \param fact the new factory
		 */
		void set_robot_controller_factory(robot_controller_factory *fact);

	private:
		clocksource &clk;
		strategy::ptr the_strategy;
		robot_controller_factory *the_rc_factory;

		void tick();
		void player_added(unsigned int, player::ptr);
		void player_removed(unsigned int, player::ptr);
};

#endif

