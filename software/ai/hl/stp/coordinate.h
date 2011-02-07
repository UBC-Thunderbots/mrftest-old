#ifndef AI_HL_STP_COORDINATE_H
#define AI_HL_STP_COORDINATE_H

#include "ai/hl/world.h"
#include "ai/hl/stp/enemy.h"
#include "ai/hl/stp/role.h"

namespace AI {
	namespace HL {
		namespace STP {
			/**
			 * See STP paper section 5.2.3 (b)
			 * Describes dynamically changing coordinate system.
			 * For example, a location relative to a moving object.
			 *
			 * To evaluate the value of a coordinate, use the operator().
			 * Coordinate A = Coordinate::relative(some_robot);
			 * Point p = A(); // gives the point.
			 */
			class Coordinate {
				public:
					/**
					 * Default constructor, gives the origin.
					 */
					Coordinate() : pos(0.0, 0.0) {
					}

					/**
					 * Copy constructor.
					 */
					Coordinate(const Coordinate& coordinate) : data(coordinate.data), pos(coordinate.pos) {
					}

					/**
					 * Implicit convertion from a point to a coordinate.
					 */
					Coordinate(const Point& off) : pos(off) {
					}

					/**
					 * Translate and rotate to enemy coordinate.
					 */
					static Coordinate relative(const AI::HL::STP::Enemy::Ptr enemy, const Point& off);

					/**
					 * Translate and rotate to player coordinate.
					 */
					static Coordinate relative(const Role::Ptr role, const Point& off);

					/**
					 * Just an offset to an object (Do not rotate coordinate).
					 */
					static Coordinate offset(const AI::HL::W::Ball& ball, const Point& off);
					static Coordinate offset(const AI::HL::STP::Enemy::Ptr enemy, const Point& off);
					static Coordinate offset(const Role::Ptr role, const Point& off);

					/**
					 * Evaluates and returns the required coordinate.
					 */
					Point operator()() const;

				protected:
					/**
					 * Stores the actual data.
					 * This data is IMMUTABLE.
					 */
					class CoordinateData : public ByRef {
						public:
							typedef RefPtr<CoordinateData> Ptr;
							virtual Point position() const;
							virtual double orientation() const;
					};

					CoordinateData::Ptr data;

					Point pos;

					explicit Coordinate(CoordinateData::Ptr d, const Point& off) : data(d), pos(off) {
					}
			};
		}
	}
}

#endif

