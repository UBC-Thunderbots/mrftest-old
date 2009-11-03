#ifndef AI_TACTIC_DANCE_H
#define AI_TACTIC_DANCE_H

#include <glibmm.h>
#include "util/byref.h"
#include "world/ball.h"
#include "world/field.h"
#include "world/player.h"
#include "world/team.h"
#include "ai/tactic.h"
#include "ai/navigator.h"

//
// Victory (?) dance.
//
class dance : public tactic {
	public:
		//
		// A pointer to a dance tactic.
		//
		typedef Glib::RefPtr<dance> ptr;

		//
		// Constructs a new dance tactic.
		//
		dance(ball::ptr ball, field::ptr field, controlled_team::ptr team, player::ptr player);

		//
		// Runs the AI for one time tick.
		//
		void update();

	protected:		

        // Keeps track of the number of times update() was called.
        // This is used as an elementary clock, to give the dance some structure.
        unsigned int tick; 
        
        // Used to control a robot, and read position.
        player::ptr the_player;
        navigator::ptr the_navigator;
};

#endif

