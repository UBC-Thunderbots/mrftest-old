#ifndef AI_ROLE_VICTORY_DANCE_H
#define AI_ROLE_VICTORY_DANCE_H

#include "ai/role.h"
#include "ai/tactic/dance.h"

//
// Gets the robots to go to their victory_dance positions.
//
class victory_dance : public role {
	public:
		//
		// A pointer to a victory_dance role.
		//
		typedef Glib::RefPtr<victory_dance> ptr;

		//
		// Constructs a new victory_dance role.
		//
		victory_dance(ball::ptr ball, field::ptr field, controlled_team::ptr team);

		//
		// Runs the AI for one time tick.
		//
		void tick();

        void set_robots(const std::vector<player::ptr> &robots);

	protected:
        std::vector<dance::ptr> the_tactics;
		
};

#endif

