#ifndef AI_HL_STP_EVALUATE_COORDINATE_H
#define AI_HL_STP_EVALUATE_COORDINATE_H

#include "ai/hl/world.h"
#include "ai/hl/stp/evaluate/enemy.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluation {
				/**
				 * See STP paper section 5.2.3 (b)
				 * Describes dynamically changing coordinate system.
				 * For example, a location relative to a moving object.
				 *
				 * This class emulates the behaviour of the Point class.
				 * Not all operations of the Point are applicable.
				 */
				class Coordinate {
					public:
						/**
						 * Default constructor, gives the origin.
						 */
						Coordinate() : offset(0.0, 0.0) {
						}

						/**
						 * Copy constructor.
						 */
						Coordinate(const Coordinate& coordinate) : data(coordinate.data), offset(coordinate.offset) {
						}

						/**
						 * Assigns one coordiante to another.
						 */
						Coordinate &operator=(const Coordinate& coordinate);

						/**
						 * Converts a point to a coordinate.
						 */
						explicit Coordinate(const Point& off) : offset(off) {
						}

						/**
						 * An enemy role.
						 */
						explicit Coordinate(const EnemyRole::Ptr enemy, const Point& off);

						/**
						 * Player coordinate, with offset.
						 */
						explicit Coordinate(const AI::HL::W::Player::Ptr player, const Point& off);

						/**
						 * Evaluates and returns the required coordinate.
						 */
						Point evaluate() const;

						/**
						 * Evaluates and returns the required coordinate.
						 */
						operator Point() const {
							return evaluate();
						}

						/**
						 * Returns an original coordinate without any offset.
						 */
						Coordinate origin() const;

						/**
						 * Adds an offset to this coordinate.
						 */
						Coordinate &operator+=(const Point& p);

						/**
						 * Removes an offset to this coordinate.
						 */
						Coordinate &operator-=(const Point& p);

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

						Point offset;

						explicit Coordinate(CoordinateData::Ptr d, const Point& off) : data(d), offset(off) {
						}
				};
			}
		}
	}
}

#endif

