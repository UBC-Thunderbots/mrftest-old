#ifndef SIM_PLAYER_H
#define SIM_PLAYER_H

#include "geom/point.h"
#include "util/memory.h"
#include "xbee/shared/packettypes.h"
#include <glibmm.h>

/**
 * A player, as seen by a simulation engine. An individual engine is expected to
 * subclass this class and return instances of the subclass from its
 * SimulatorEngine::add_player(bool) method.
 */
class SimulatorPlayer : public ByRef {
	public:
		/**
		 * \return the position of the player, in metres from field centre
		 */
		virtual Point position() const = 0;

		/**
		 * Moves the player.
		 * \param pos the new position, in metres from field centre
		 */
		virtual void position(const Point &pos) = 0;

		/**
		 * \return the orientation of the player, in radians from field east
		 */
		virtual double orientation() const = 0;

		/**
		 * \return The speed of the dribbler roller, in revolutions per ten
		 * milliseconds
		 */
		virtual unsigned int dribbler_speed() const = 0;

		/**
		 * Reorients the player.
		 * \param ori the new orientation, in radians from field east
		 */
		virtual void orientation(double ori) = 0;

		/**
		 * Sets the velocity of the player.
		 * \param vel the new velocity, in metres per second field-relative
		 */
		virtual void velocity(const Point &vel) = 0;

		/**
		 * Sets the angular velocity of the player.
		 * \param avel the new angular velocity, in radians per second
		 */
		virtual void avelocity(double avel) = 0;

		/**
		 * Handles a "radio" packet received from the AI.
		 * \param packet the packet
		 */
		virtual void received(const XBeePacketTypes::RUN_DATA &packet) = 0;
};

#endif

