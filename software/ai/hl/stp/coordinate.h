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
			 *
			 * Essentially this class is just a wrapper around a function object that returns a point.
			 */
			class Coordinate {
				public:
					/**
					 * A function object returning a point.
					 */
					class CoordinateData : public ByRef {
						public:
							CoordinateData() {}
							~CoordinateData() {}
							virtual Point evaluate() const = 0;
					};
				
					/**
					 * Default constructor, gives the origin.
					 */
					Coordinate();

					/**
					 * Copy constructor.
					 */
					Coordinate(const Coordinate& coordinate) : data(coordinate.data) {
					}

					/**
					 * Implicit convertion from a point to a coordinate.
					 */
					Coordinate(const Point& pos);

					/**
					 * If the y coordinate of the ball is less than 0,
					 * flips the y coordinate.
					 */
					static Coordinate ball_up(const AI::HL::W::Ball& ball, const Point& pos);

					/**
					 * Translate and rotate to enemy coordinate.
					 */
					static Coordinate relative(const Enemy::Ptr enemy, const Point& off);

					/**
					 * Translate and rotate to player coordinate.
					 */
					static Coordinate relative(const Role::Ptr role, const Point& off);

					/**
					 * Just an offset to a ball (Do not rotate coordinate).
					 */
					static Coordinate offset(const AI::HL::W::Ball& ball, const Point& off);

					/**
					 * Just an offset to an enemy (Do not rotate coordinate).
					 */
					static Coordinate offset(const Enemy::Ptr enemy, const Point& off);

					/**
					 * Just an offset to a player (Do not rotate coordinate).
					 */
					static Coordinate offset(const Role::Ptr role, const Point& off);

					/**
					 * Evaluates and returns the required coordinate.
					 */
					Point operator()() const;

				protected:
					RefPtr<CoordinateData> data;

					Coordinate(const RefPtr<CoordinateData> data);
			};
		}
	}
}

#endif

