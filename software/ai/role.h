#ifndef AI_ROLE_H
#define AI_ROLE_H

#include "world/ball.h"
#include "world/field.h"
#include "world/team.h"

//
// A role manages the operation of a small group of players.
//
class role : public byref, public sigc::trackable {
	public:
		//
		// A pointer to a role.
		//
		typedef Glib::RefPtr<role> ptr;

		//
		// Constructs a new role. Call this constructor from subclass constructors.
		//
		role(ball::ptr ball, field::ptr field, controlled_team::ptr team);

		//
		// Runs the role for one time tick. It is expected that the role will
		// examine the robots for which it is responsible, determine if they
		// need to be given new tactics, and then call tactic::tick() for all
		// the tactics under this role.
		//
		// It is possible that the set of robots controlled by the tactic may
		// change between one tick and the next. The role must be ready to deal
		// with this situation, and must be sure to destroy any tactics
		// controlling robots that have gone away.
		//
		virtual void tick() = 0;
		
		//
		// Sets the robots controlled by this role.
		//
		void set_robots(const std::vector<player::ptr> &robots);
		
		//
		// Removes all robots from this role.
		//
		void clear_robots();

	protected:
		//
		// The ball.
		//
		const ball::ptr the_ball;

		//
		// The field.
		//
		const field::ptr the_field;

		//
		// The team this role controls.
		//
		const controlled_team::ptr the_team;
		
		//
		// The  robots that this role controls.
		//
		std::vector<player::ptr> the_robots;
};

#endif

