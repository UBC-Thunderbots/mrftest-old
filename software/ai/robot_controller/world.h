#ifndef AI_ROBOT_CONTROLLER_WORLD_H
#define AI_ROBOT_CONTROLLER_WORLD_H

#include "ai/common/player.h"
#include "geom/point.h"
#include "util/byref.h"
#include "util/time.h"
#include <utility>
#include <vector>

namespace AI {
	namespace RC {
		namespace W {
			/**
			 * A player, as seen by a RobotController.
			 */
			class Player : public AI::Common::Player, public AI::Common::Robot {
				public:
					/**
					 * A pointer to a Player.
					 */
					typedef RefPtr<Player> Ptr;

					/**
					 * Returns the path requested by the navigator.
					 *
					 * \return the path, in the form of a set of
					 * ((<var>position</var>, <var>orientation</var>), <var>deadline</var>) pairs,
					 * where <var>deadline</var> is the timestamp at which the robot should arrive.
					 */
					virtual const std::vector<std::pair<std::pair<Point, double>, timespec> > &path() const = 0;

					/**
					 * Orders the wheels to turn at specified speeds.
					 *
					 * \param[in] w the speeds of the four wheels,
					 * in order front-left, back-left, back-right, front-right,
					 * in units of quarters of a degree of motor shaft rotation per five milliseconds.
					 */
					virtual void drive(const int(&w)[4]) = 0;
			};
		}
	}
}

#endif

