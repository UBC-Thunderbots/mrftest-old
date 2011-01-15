#ifndef AI_HL_STP_EVALUATE_COORDINATE_H
#define AI_HL_STP_EVALUATE_COORDINATE_H

#include "ai/hl/world.h"
#include "util/cacheable.h"

namespace AI {
	namespace HL {
		namespace STP {
			namespace Evaluate {
				/**
				 * Describes the types of coordinate.
				 * See STP paper section 5.2.3 (b)
				 */
				enum CoordinateType {
					/**
					 * Unmodified game coordinate.
					 */
					COORDINATE_ABSOLUTE,

					/**
					 * Relative to a particular player.
					 * This coordinate is also rotated in the player's direction.
					 */
					COORDINATE_RELATIVE_PLAYER,

					/**
					 * Relative to a particular robot.
					 * This coordinate is also rotated in the robots's direction.
					 */
					COORDINATE_RELATIVE_ROBOT,

					/**
					 * Ball relative position.
					 */
					COORDINATE_RELATIVE_BALL,

					/**
					 * Relative to a particular position.
					 * May also be rotated.
					 */
					COORDINATE_RELATIVE_POSITION,

					/**
					 * Offset of some other coordinate.
					 */
					COORDINATE_OFFSET,
				};

				/**
				 * Describes dynamically changing coordinate system.
				 * This emulates STP paper section 5.2.3 (b)
				 */
				class Coordinate : public ByRef {
					public:
						typedef RefPtr<Coordinate> Ptr;

						/**
						 * Evaluates and returns the required coordinate.
						 */
						virtual Point evaluate() const = 0;

						/**
						 * Offset of some relative coordinate.
						 */
						virtual Coordinate::Ptr offset(const Point& off) = 0;

						static Coordinate::Ptr absolute(const Point& point);

						static Coordinate::Ptr relative_player(const AI::HL::W::Player::Ptr player, const Point& p);

						static Coordinate::Ptr relative_robot(const AI::HL::W::Robot::Ptr robot, const Point& p);

						static Coordinate::Ptr relative_ball(const AI::HL::W::Ball& ball);

						static Coordinate::Ptr relative(const Point& pos, const double ori);

					protected:
						Coordinate(CoordinateType t, const Point& off);
						CoordinateType type_;
						Point offset_;
				};

				namespace {
					/**
					 * Adds an offset to a Coordinate.
					 *
					 * \param[in] coord the Coordinate.
					 *
					 * \param[in] p the point.
					 */
					Coordinate::Ptr operator+(const Coordinate::Ptr coord, const Point& p) __attribute__((warn_unused_result));
					Coordinate::Ptr operator+(const Coordinate::Ptr coord, const Point& p) {
						return coord->offset(p);
					}
				}
			}
		}
	}
}

#endif

