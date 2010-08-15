#ifndef AI_AI_H
#define AI_AI_H

#include "ai/coach/coach.h"
#include "ai/world/world.h"
#include "robot_controller/robot_controller.h"
#include "util/clocksource.h"
#include "util/noncopyable.h"

/**
 * A complete %AI.
 */
class AI : public NonCopyable {
	public:
		/**
		 * The World in which the AI is running.
		 */
		const World::Ptr world;

		/**
		 * Constructs a new AI.
		 *
		 * \param[in] world the World to operate in.
		 *
		 * \param[in] clk the clock to run the AI from.
		 */
		AI(const World::Ptr &world, ClockSource &clk);

		/**
		 * Gets the Coach managing the AI.
		 *
		 * \return the Coach managing the AI.
		 */
		Coach::Ptr get_coach() const;

		/**
		 * Sets which Coach is managing the AI.
		 *
		 * \param[in] c the new Coach.
		 */
		void set_coach(Coach::Ptr c);

		/**
		 * Gets the RobotControllerFactory driving the robots.
		 *
		 * \return the RobotControllerFactory driving the robots.
		 */
		RobotControllerFactory *get_robot_controller_factory() const {
			return rc_factory;
		}

		/**
		 * Sets the RobotControllerFactory that creates \ref RobotController
		 * "RobotControllers" that drive the robots.
		 *
		 * \param[in] fact the new factory.
		 */
		void set_robot_controller_factory(RobotControllerFactory *fact);

	private:
		ClockSource &clk;
		Coach::Ptr coach;
		RobotControllerFactory *rc_factory;

		void tick();
		void player_added(unsigned int, Player::Ptr);
		void player_removed(unsigned int, Player::Ptr);
};

#endif

