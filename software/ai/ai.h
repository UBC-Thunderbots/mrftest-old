#ifndef AI_AI_H
#define AI_AI_H

#include "ai/backend/backend.h"
#include "ai/hl/hl.h"
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
			 * The HighLevel in use.
			 */
			Property<std::unique_ptr<AI::HL::HighLevel> > high_level;

			/**
			 * The Navigator navigating the robots.
			 */
			Property<AI::Nav::Navigator::Ptr> navigator;

			/**
			 * The RobotControllerFactory driving the robots.
			 */
			Property<AI::RC::RobotControllerFactory *> robot_controller_factory;

			/**
			 * \brief Whether or not the overlay mechanism should render the high-level overlay.
			 */
			bool show_hl_overlay;

			/**
			 * \brief Whether or not the overlay mechanism should render the navigator overlay.
			 */
			bool show_nav_overlay;

			/**
			 * \brief Whether or not the overlay mechanism should render the robot controller overlay.
			 */
			bool show_rc_overlay;

			/**
			 * Constructs a new AIPackage.
			 *
			 * \param[in] backend the Backend against which to run.
			 */
			AIPackage(AI::BE::Backend &backend);

		private:
			void tick();
			void attach_robot_controllers();
			void robot_controller_factory_changed();
			void draw_overlay(Cairo::RefPtr<Cairo::Context> ctx) const;
			void save_setup() const;
	};
}

#endif

