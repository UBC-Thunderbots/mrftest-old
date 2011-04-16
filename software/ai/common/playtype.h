#ifndef AI_COMMON_PLAYTYPE_H
#define AI_COMMON_PLAYTYPE_H

#include <functional>
#include <glibmm.h>
#include <vector>

namespace AI {
	namespace Common {
		/**
		 * \brief A state of play.
		 */
		enum class PlayType {
			/**
			 * \brief Robots are not permitted to move.
			 */
			HALT,

			/**
			 * \brief Robots are allowed anywhere outside a circle around the ball.
			 */
			STOP,

			/**
			 * \brief Game is running normally.
			 */
			PLAY,

			/**
			 * \brief Robot gets ready to take a kickoff.
			 */
			PREPARE_KICKOFF_FRIENDLY,

			/**
			 * \brief Kickoff is occurring but ball has not yet moved.
			 */
			EXECUTE_KICKOFF_FRIENDLY,

			/**
			 * \brief Robot gets ready to take a kickoff.
			 */
			PREPARE_KICKOFF_ENEMY,

			/**
			 * \brief Kickoff is occurring but ball has not yet moved.
			 */
			EXECUTE_KICKOFF_ENEMY,

			/**
			 * \brief Robot gets ready to take a penalty kick.
			 */
			PREPARE_PENALTY_FRIENDLY,

			/**
			 * \brief Penalty kick is occurring but ball has not yet moved.
			 */
			EXECUTE_PENALTY_FRIENDLY,

			/**
			 * \brief Robot gets ready to take a penalty kick.
			 */
			PREPARE_PENALTY_ENEMY,

			/**
			 * \brief Penalty kick is occurring but ball has not yet moved.
			 */
			EXECUTE_PENALTY_ENEMY,

			/**
			 * \brief Direct free kick is occurring but ball has not yet moved.
			 */
			EXECUTE_DIRECT_FREE_KICK_FRIENDLY,

			/**
			 * \brief Indirect free kick is occurring but ball has not yet moved.
			 */
			EXECUTE_INDIRECT_FREE_KICK_FRIENDLY,

			/**
			 * \brief Direct free kick is occurring but ball has not yet moved.
			 */
			EXECUTE_DIRECT_FREE_KICK_ENEMY,

			/**
			 * \brief Indirect free kick is occurring but ball has not yet moved.
			 */
			EXECUTE_INDIRECT_FREE_KICK_ENEMY,

			/**
			 * \brief There is no play type.
			 *
			 * This should be the last play type in the list.
			 */
			NONE,
		};

		namespace PlayTypeInfo {
			/**
			 * \brief Converts an integer index to a play type.
			 *
			 * \param[in] pt the integer to convert.
			 *
			 * \return the play type.
			 */
			PlayType of_int(unsigned int pt);

			/**
			 * \brief Returns a description of a play type.
			 *
			 * \param[in] pt the play type.
			 *
			 * \return the description.
			 */
			Glib::ustring to_string(PlayType pt);

			/**
			 * \brief Inverts the sense of a play type.
			 *
			 * Inverting a play type causes any play type which refers to “friendly” to refer to “enemy” and vice versa.
			 * Other play types are not affected.
			 *
			 * \param[in] pt the play type.
			 *
			 * \return the inverted play type.
			 */
			PlayType invert(PlayType pt);
		}
	}
}

namespace std {
	template<> struct hash<AI::Common::PlayType> {
		std::hash<unsigned int> h;

		std::size_t operator()(AI::Common::PlayType pt) const {
			return h(static_cast<unsigned int>(pt));
		}
	};
}

#endif

