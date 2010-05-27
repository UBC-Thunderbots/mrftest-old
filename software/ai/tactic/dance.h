#ifndef AI_TACTIC_DANCE_H
#define AI_TACTIC_DANCE_H

#include "ai/tactic/tactic.h"

//
// Victory (?) dance.
//
class dance : public tactic {
	public:
		//
		// A pointer to this tactic.
		//
		typedef Glib::RefPtr<dance> ptr;

		//
		// Constructs a new dance tactic.
		//
		dance(player::ptr player);

		//
		// Runs the AI for one time tick.
		//
		void tick();

	protected:		

        // Keeps track of the number of times tick() was called.
        // This is used as an elementary clock, to give the dance some structure.
        unsigned int ticks;
        
};

#endif

