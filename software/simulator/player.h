#ifndef SIM_PLAYER_H
#define SIM_PLAYER_H

#include "geom/point.h"
#include "simulator/sockproto/proto.h"
#include "util/byref.h"

namespace Simulator {
	/**
	 * A player, as seen by a simulation engine.
	 * An individual engine is expected to subclass this class and return instances of the subclass from its SimulatorEngine::add_player() method.
	 */
	class Player : public ByRef {
		public:
			/**
			 * A pointer to a Player.
			 */
			typedef RefPtr<Player> Ptr;

			/**
			 * The most recent set of orders issued by the AI to the player.
			 * The engine should examine these orders when running a time tick.
			 */
			Simulator::Proto::A2SPlayerInfo orders;

			/**
			 * Returns the player's position.
			 *
			 * \return the position of the player, in metres from field centre.
			 */
			virtual Point position() const = 0;

			/**
			 * Moves the player.
			 *
			 * \param[in] pos the new position, in metres from field centre.
			 */
			virtual void position(const Point &pos) = 0;

			/**
			 * Returns the player's orientation.
			 *
			 * \return the orientation of the player, in radians from field east.
			 */
			virtual double orientation() const = 0;

			/**
			 * Reorients the player.
			 *
			 * \param[in] ori the new orientation, in radians from field east.
			 */
			virtual void orientation(double ori) = 0;

			/**
			 * Checks whether the player has the ball.
			 *
			 * \return \c true if the player has the ball, or \c false if not.
			 */
			virtual bool has_ball() const = 0;
	};
}

#endif

