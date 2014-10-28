#ifndef AI_HL_STP_COORDINATE_H
#define AI_HL_STP_COORDINATE_H

#include "geom/point.h"
#include "ai/hl/stp/world.h"
#include <cstddef>
#include <functional>
#include <memory>

namespace AI {
	namespace HL {
		namespace STP {
			/**
			 * See STP paper section 5.2.3 (b)
			 * Describes dynamically changing coordinate system.
			 * For example, a location relative to a moving object.
			 */
			class Coordinate final {
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
						OUR_SIDE_STRONG,

						/**
						 * Side with more of THEIR players.
						 */
						THEIR_SIDE_STRONG,
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

					explicit Coordinate();

					/**
					 * Implicit conversion.
					 */
					Coordinate(Point pos);

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
					explicit Coordinate(World world, Point pos, YType y_type, OriginType o_type);

					/**
					 * Evaluates the absolute position.
					 */
					Point position() const;

					/**
					 * Evaluates the absolute velocity.
					 */
					Point velocity() const;

					Coordinate &operator=(const Coordinate &c);

					bool operator==(const Coordinate &other) const;

					std::size_t hash() const;

				protected:
					std::unique_ptr<World> world;
					YType y_type;
					OriginType o_type;
					Point pos;
			};
		}
	}
}

namespace std {
	template<> struct hash<AI::HL::STP::Coordinate> final {
		typedef AI::HL::STP::Coordinate argument_type;
		typedef std::size_t result_type;

		std::size_t operator()(const AI::HL::STP::Coordinate &c) const {
			return c.hash();
		}
	};
}

#endif

