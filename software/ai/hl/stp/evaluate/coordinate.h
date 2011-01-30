#ifndef AI_HL_STP_EVALUATE_COORDINATE_H
#define AI_HL_STP_EVALUATE_COORDINATE_H

#include "ai/hl/world.h"
#include "util/cacheable.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluate {
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
						Coordinate();

						/**
						 * Copy constructor.
						 */
						Coordinate(const Coordinate& coordinate);

						/**
						 * Assigns one coordiante to another.
						 */
						Coordinate &operator=(const Coordinate& coordinate);

						/**
						 * Converts a point to a coordinate.
						 */
						explicit Coordinate(const Point& off);

						/**
						 * Relative to a robot.
						 */
						explicit Coordinate(const AI::HL::W::Robot::Ptr robot, const Point& off);

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
				};
			}
		}
	}
}

#endif

