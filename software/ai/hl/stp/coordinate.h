#ifndef AI_HL_STP_COORDINATE_H
#define AI_HL_STP_COORDINATE_H

#include "geom/point.h"
#include "ai/hl/stp/world.h"

namespace AI {
	namespace HL {
		namespace STP {
			/**
			 * See STP paper section 5.2.3 (b)
			 * Describes dynamically changing coordinate system.
			 * For example, a location relative to a moving object.
			 */
			class Coordinate {
				public:
					/**
					 * Determines the y-coordinate.
					 */
					enum class YType {
						/**
						 * Use the normal coordinate system.
						 */
						ABSOLUTE,

						/**
						 * Ball is always up-direction
						 */
						BALL,

						/**
						 * Side with more of OUR players.
						 */
						OUR_MAJORITY,

						/**
						 * Side with more of THEIR players.
						 */
						THEIR_MAJORITY,
					};

					/**
					 * Determines the origin.
					 */
					enum class OriginType {
						/**
						 * Center of field.
						 */
						ABSOLUTE,

						/**
						 * Ball is origin.
						 */
						BALL,
					};

					/**
					 * Implicit conversion.
					 */
					Coordinate(const Point &pos);

					/**
					 * Copy constructor.
					 */
					Coordinate(const Coordinate &coord);

					/**
					 * Main constructor.
					 *
					 * \param [in] pos the offset from the origin
					 *
					 * \param [in] y_type determines the direction y-axis is pointing.
					 *
					 * \param [in] o_type determines the origin.
					 */
					Coordinate(const World &world, const Point &pos, YType y_type, OriginType o_type);

					/**
					 * Evaluates the absolute position.
					 */
					Point position() const;

					/**
					 * Evaluates the absolute velocity.
					 */
					Point velocity() const;

				protected:
					const World *const world;
					const YType y_type;
					const OriginType o_type;
					const Point pos;
			};
		}
	}
}

#endif

