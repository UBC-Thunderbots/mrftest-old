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
			 *
			 * To evaluate the value of a coordinate, use the operator().
			 * Point p = A(); // gives the point.
			 *
			 * Essentially this class is just a wrapper around a function object that returns a point.
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
						 * Side with more of our players.
						 */
						MAJORITY,
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
					Coordinate(const Point& pos);

					/**
					 * Main constructor
					 */
					Coordinate(const World& world, const Point& pos, const YType y_type, const OriginType o_type);

					/**
					 * Evaluates and returns the required coordinate.
					 */
					Point operator()() const;

				protected:
					const World* world;
					const YType y_type;
					const OriginType o_type;
					const Point& pos;
			};
		}
	}
}

#endif

