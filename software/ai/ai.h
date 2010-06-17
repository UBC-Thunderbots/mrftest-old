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
		 * \param[in] world the world to operate in.
		 *
		 * \param[in] clk the clock to run the AI from.
		 */
		ai(world::ptr world, clocksource &clk);

		/**
		 * \return the strategy driving the robots.
		 */
		strategy::ptr get_strategy() const {
			return the_strategy;
		}

		/**
		 * Sets which strategy is driving the robots.
		 *
		 * \param[in] strat the new strategy.
		 */
		void set_strategy(strategy::ptr strat);

		/**
		 * \return the robot_controller_factory driving the robots.
		 */
		robot_controller_factory *get_robot_controller_factory() const {
			return the_rc_factory;
		}

		/**
		 * Sets the robot_controller_factory that creates robot_controllers that
		 * drive the robots.
		 *
		 * \param[in] fact the new factory.
		 */
		void set_robot_controller_factory(robot_controller_factory *fact);

		/**
		 * Sets the overlay context onto which the strategy should draw to
		 * display data in the visualizer.
		 *
		 * \param[in] ovl the new overlay.
		 */
		void set_overlay(Cairo::RefPtr<Cairo::Context> ovl);

	private:
		clocksource &clk;
		strategy::ptr the_strategy;
		robot_controller_factory *the_rc_factory;
		Cairo::RefPtr<Cairo::Context> overlay;

		void tick();
		void player_added(unsigned int, player::ptr);
		void player_removed(unsigned int, player::ptr);
};

#endif

