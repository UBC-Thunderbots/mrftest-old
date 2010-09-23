#ifndef AI_COMMON_ROBOT_H
#define AI_COMMON_ROBOT_H

#include "ai/common/predictable.h"
#include "util/byref.h"
#include "util/objectstore.h"

namespace AI {
	namespace Common {
		/**
		 * The common functions available on a robot in all layers.
		 */
		class Robot : public OrientationPredictable, public virtual ByRef {
			public:
				/**
				 * The largest possible radius of a robot, in metres.
				 */
				static const double MAX_RADIUS;

				/**
				 * Returns the index of the robot.
				 *
				 * \return the index of the robot's lid pattern.
				 */
				virtual unsigned int pattern() const = 0;

				/**
				 * Returns an object store for the robot.
				 * AI entities can use this object store to hold opaque data on a per-robot basis,
				 * without worrying about keeping parallel data structures up-to-date and dealing with team changes.
				 *
				 * \return an object store.
				 */
				virtual ObjectStore &object_store() = 0;
		};
	}
}

#endif

