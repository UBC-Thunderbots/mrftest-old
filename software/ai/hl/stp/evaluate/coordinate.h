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

						/**
						 * Absolute game coordinate.
						 */
						static Coordinate::Ptr absolute(const Point& point);

						/**
						 * Relative to a particular player, even its orientation.
						 */
						static Coordinate::Ptr relative_player(const AI::HL::W::Player::Ptr player, const Point& p);

						/**
						 * Relative to a particular robot, even its orientation.
						 */
						static Coordinate::Ptr relative_robot(const AI::HL::W::Robot::Ptr robot, const Point& p);

						/**
						 * Relative to the ball.
						 * There is no rotation.
						 */
						static Coordinate::Ptr relative_ball(const AI::HL::W::Ball& ball);

						/**
						 * A transformed coordinate.
						 */
						static Coordinate::Ptr transform(const Point& pos, const double ori);

					protected:
						Coordinate(const Point& off);
						Point offset_;
						// maybe we need rotation as well.
				};
			}
		}
	}
}

#endif

