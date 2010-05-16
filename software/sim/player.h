#ifndef SIM_PLAYER_H
#define SIM_PLAYER_H

#include "geom/point.h"
#include "util/byref.h"
#include "xbee/shared/packettypes.h"
#include <glibmm.h>

/**
 * A player, as seen by a simulation engine. An individual engine is expected to
 * subclass this class and return instances of the subclass from its
 * simulator_engine::add_player(bool) method.
 */
class player : public byref {
	public:
		/**
		 * A pointer to a player.
		 */
		typedef Glib::RefPtr<player> ptr;

		/**
		 * \return the position of the player, in metres from field centre
		 */
		virtual point position() const = 0;

		/**
		 * Moves the player.
		 * \param pos the new position, in metres from field centre
		 */
		virtual void position(const point &pos) = 0;

		/**
		 * \return the orientation of the player, in radians from field east
		 */
		virtual double orientation() const = 0;

		/**
		 * \return true if the player has possession of the ball, or false if
		 * not
		 */
		virtual bool has_ball() const = 0;

		/**
		 * Reorients the player.
		 * \param ori the new orientation, in radians from field east
		 */
		virtual void orientation(double ori) = 0;

		/**
		 * Sets the velocity of the player.
		 * \param vel the new velocity, in metres per second field-relative
		 */
		virtual void velocity(const point &vel) = 0;

		/**
		 * Sets the angular velocity of the player.
		 * \param avel the new angular velocity, in radians per second
		 */
		virtual void avelocity(double avel) = 0;

		/**
		 * Handles a "radio" packet received from the AI.
		 * \param packet the packet
		 */
		virtual void received(const xbeepacket::RUN_DATA &packet) = 0;
};

#endif

