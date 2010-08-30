#ifndef AI_WORLD_PLAYTYPE_H
#define AI_WORLD_PLAYTYPE_H

#include <glibmm.h>

namespace PlayType {
	/**
	 * A state of play.
	 */
	enum PlayType {
		/**
		 * Robots are not permitted to move.
		 */
		HALT,

		/**
		 * Robots are allowed anywhere outside a circle around the ball.
		 */
		STOP,

		/**
		 * Game is running normally.
		 */
		PLAY,

		/**
		 * Robot gets ready to take a kickoff.
		 */
		PREPARE_KICKOFF_FRIENDLY,

		/**
		 * Kickoff is occurring but ball has not yet moved.
		 */
		EXECUTE_KICKOFF_FRIENDLY,

		/**
		 * Robot gets ready to take a kickoff.
		 */
		PREPARE_KICKOFF_ENEMY,

		/**
		 * Kickoff is occurring but ball has not yet moved.
		 */
		EXECUTE_KICKOFF_ENEMY,

		/**
		 * Robot gets ready to take a penalty kick.
		 */
		PREPARE_PENALTY_FRIENDLY,

		/**
		 * Penalty kick is occurring but ball has not yet moved.
		 */
		EXECUTE_PENALTY_FRIENDLY,

		/**
		 * Robot gets ready to take a penalty kick.
		 */
		PREPARE_PENALTY_ENEMY,

		/**
		 * Penalty kick is occurring but ball has not yet moved.
		 */
		EXECUTE_PENALTY_ENEMY,

		/**
		 * Direct free kick is occurring but ball has not yet moved.
		 */
		EXECUTE_DIRECT_FREE_KICK_FRIENDLY,

		/**
		 * Indirect free kick is occurring but ball has not yet moved.
		 */
		EXECUTE_INDIRECT_FREE_KICK_FRIENDLY,

		/**
		 * Direct free kick is occurring but ball has not yet moved.
		 */
		EXECUTE_DIRECT_FREE_KICK_ENEMY,

		/**
		 * Indirect free kick is occurring but ball has not yet moved.
		 */
		EXECUTE_INDIRECT_FREE_KICK_ENEMY,

		/**
		 * A counter of the number of legal playtypes.
		 * Put new types before this.
		 */
		COUNT,
	};

	/*
	 * Descriptions of the play types from a generic team's point of view.
	 * These descriptions simply use the words "friendly" and "enemy", and are appropriate for the AI application.
	 */
	extern const Glib::ustring DESCRIPTIONS_GENERIC[COUNT];

	/*
	 * Descriptions of the play types from the west team's point of view.
	 * These descriptions use the words "west" and "east", and are appropriate for the simulator application.
	 */
	extern const Glib::ustring DESCRIPTIONS_WEST[COUNT];

	/*
	 * Directional inversions of the play types.
	 * An element in this array contains the play type as the east team would see it,
	 * if the index into the array is the play type as the west team would see it.
	 */
	extern const PlayType INVERT[COUNT];
}

#endif

