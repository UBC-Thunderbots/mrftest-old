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
class AI : public NonCopyable {
	public:
		/**
		 * The world in which the AI is running.
		 */
		const RefPtr<World> world;

		/**
		 * Constructs a new AI.
		 *
		 * \param[in] world the world to operate in.
		 *
		 * \param[in] clk the clock to run the AI from.
		 */
		AI(RefPtr<World> world, ClockSource &clk);

		/**
		 * \return the Strategy driving the robots.
		 */
		RefPtr<Strategy2> get_strategy() const {
			return strategy;
		}

		/**
		 * Sets which Strategy is driving the robots.
		 *
		 * \param[in] strat the new Strategy.
		 */
		void set_strategy(RefPtr<Strategy2> strat);

		/**
		 * \return the RobotControllerFactory driving the robots.
		 */
		RobotControllerFactory *get_robot_controller_factory() const {
			return rc_factory;
		}

		/**
		 * Sets the RobotControllerFactory that creates robot_controllers that
		 * drive the robots.
		 *
		 * \param[in] fact the new factory.
		 */
		void set_robot_controller_factory(RobotControllerFactory *fact);

		/**
		 * Sets the overlay context onto which the Strategy should draw to
		 * display data in the visualizer.
		 *
		 * \param[in] ovl the new overlay.
		 */
		void set_overlay(Cairo::RefPtr<Cairo::Context> ovl);

	private:
		ClockSource &clk;
		RefPtr<Strategy2> strategy;
		RobotControllerFactory *rc_factory;
		Cairo::RefPtr<Cairo::Context> overlay;

		void tick();
		void player_added(unsigned int, RefPtr<Player>);
		void player_removed(unsigned int, RefPtr<Player>);
};

#endif

