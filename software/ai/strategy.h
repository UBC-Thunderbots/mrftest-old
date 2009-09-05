#ifndef AI_STRATEGY_H
#define AI_STRATEGY_H

#include <glibmm.h>
#include "util/byref.h"
#include "world/ball.h"
#include "world/field.h"
#include "world/team.h"

//
// A strategy manages the overall operation of a team.
//
class strategy : public virtual byref {
	public:
		//
		// A pointer to a strategy.
		//
		typedef Glib::RefPtr<strategy> ptr;

		//
		// Constructs a new strategy. Call this constructor from subclass constructors.
		//
		strategy(ball::ptr ball, field::ptr field, controlled_team::ptr team);

		//
		// Runs the AI for one time tick.
		//
		virtual void update() = 0;

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
		// The team this strategy controls.
		//
		const controlled_team::ptr the_team;
};

#endif

