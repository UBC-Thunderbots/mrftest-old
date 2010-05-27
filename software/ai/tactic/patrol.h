#ifndef AI_TACTIC_PATROL_H
#define AI_TACTIC_PATROL_H

#include "ai/tactic/tactic.h"
#include "ai/tactic/move.h"

/**
 * This assumes that the role does not get recreated and that this tactic persists for the duration of the role using this tactic.
 */
class patrol : public tactic {
	public:
		//
		// A pointer to this tactic.
		//
		typedef Glib::RefPtr<patrol> ptr;

		/**
		 * Standard constructor.
		 */
		patrol(player::ptr player, world::ptr world, const unsigned int& flags);

		/**
		 * Most usage of move tactic only sets position and should thus justify existence of this overloaded constructor.
		 * \param position1 The first position for patrol
		 * \param position2 The second position for patrol
		 */
		patrol(player::ptr player, world::ptr world, const unsigned int& flags, const point& position1, const point& position2);

		/**
		 * Set the targets for the patrol.
		 */
		void set_targets(const point& position1, const point& position2);

		//
		// Runs the AI for one time tick.
		//
		void tick();

	protected:		
		bool should_move_to_first;

		point the_position1;
		point the_position2;

	private:
		move::ptr move_tactic1;
		move::ptr move_tactic2;
};

#endif

