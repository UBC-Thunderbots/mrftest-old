#ifndef WORLD_PLAYTYPE_H
#define WORLD_PLAYTYPE_H

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
	};
}

#endif

