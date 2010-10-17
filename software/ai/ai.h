#ifndef AI_AI_H
#define AI_AI_H

#include "ai/backend/backend.h"
#include "ai/coach/coach.h"
#include "ai/navigator/navigator.h"
#include "ai/robot_controller/robot_controller.h"
#include "util/noncopyable.h"
#include "util/property.h"

namespace AI {
	/**
	 * A complete %AI.
	 */
	class AIPackage : public NonCopyable {
		public:
			/**
			 * The Backend against which the AI is running.
			 */
			AI::BE::Backend &backend;

			/**
			 * The Coach managing the AI.
			 */
			Property<AI::Coach::Coach::Ptr> coach;

			/**
			 * The Navigator navigating the robots.
			 */
			Property<AI::Nav::Navigator::Ptr> navigator;

			/**
			 * The RobotControllerFactory driving the robots.
			 */
			Property<AI::RC::RobotControllerFactory *> robot_controller_factory;

			/**
			 * Constructs a new AIPackage.
			 *
			 * \param[in] backend the Backend against which to run.
			 */
			AIPackage(AI::BE::Backend &backend);

			/**
			 * Destroys the AIPackage.
			 */
			~AIPackage();

		private:
			void tick();
			void player_added(std::size_t idx);
			void player_removing(std::size_t idx);
			void robot_controller_factory_changed();
	};
}

#endif

