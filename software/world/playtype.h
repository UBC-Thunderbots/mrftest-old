#ifndef WORLD_PLAYTYPE_H
#define WORLD_PLAYTYPE_H

#include <glibmm.h>

namespace playtype {
	//
	// A state of play.
	//
	enum playtype {
		// Robots are not permitted to move.
		halt,

		// Robots are allowed anywhere outside a circle around the ball.
		stop,

		// Game is running normally.
		play,

		// Robot gets ready to take a kickoff.
		prepare_kickoff_friendly,

		// Kickoff is occurring but ball has not yet moved.
		execute_kickoff_friendly,

		// Robot gets ready to take a kickoff.
		prepare_kickoff_enemy,

		// Kickoff is occurring but ball has not yet moved.
		execute_kickoff_enemy,

		// Robot gets ready to take a penalty kick.
		prepare_penalty_friendly,

		// Penalty kick is occurring but ball has not yet moved.
		execute_penalty_friendly,

		// Robot gets ready to take a penalty kick.
		prepare_penalty_enemy,

		// Penalty kick is occurring but ball has not yet moved.
		execute_penalty_enemy,

		// Direct free kick is occurring but ball has not yet moved.
		execute_direct_free_kick_friendly,

		// Indirect free kick is occurring but ball has not yet moved.
		execute_indirect_free_kick_friendly,

		// Direct free kick is occurring but ball has not yet moved.
		execute_direct_free_kick_enemy,

		// Indirect free kick is occurring but ball has not yet moved.
		execute_indirect_free_kick_enemy,

		// Robots move to the edge of the field.
		pit_stop,

		// Robots do fancy stuff.
		victory_dance,

		// A counter of the number of legal playtypes. Put new types
		// before this.
		count,
	};

	//
	// Descriptions of the play types from a generic team's point of view.
	// These descriptions simply use the words "friendly" and "enemy", and
	// are appropriate for the real-world system.
	//
	extern const Glib::ustring descriptions_generic[];

	//
	// Descriptions of the play types from the west team's point of view.
	// These descriptions use the words "west" and "east", and are appropriate
	// for the simulator.
	//
	extern const Glib::ustring descriptions_west[];

	//
	// Directional inversions of the play types. An element in this array
	// contains the play type as the east team would see it, if thei ndex into
	// the array is the play type as the west team would see it.
	//
	extern const playtype invert[];
}

#endif

