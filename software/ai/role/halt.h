#ifndef AI_ROLE_HALT_H
#define AI_ROLE_HALT_H

#include "ai/role/role.h"

/**
 * Robots in this role should stop moving.
 */
class halt : public role {
	public:
		/*
		 * A pointer to a halt role.
		 */
		typedef Glib::RefPtr<halt> ptr;

		/**
		 * Constructs a new halt role.
		 */
		halt();

		/**
		 * Runs the AI for one time tick.
		 */
		void tick();

		/**
		 * Handles changes to the robot membership.
		 */
		void robots_changed();

};

#endif

