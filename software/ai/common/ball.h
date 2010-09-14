#ifndef AI_COMMON_BALL_H
#define AI_COMMON_BALL_H

#include "ai/common/predictable.h"

namespace AI {
	namespace Common {
		/**
		 * The common functions available on the ball in all layers.
		 */
		class Ball : public Predictable {
			public:
				/**
				 * The approximate radius of the ball.
				 */
				static const double RADIUS;
		};
	}
}

#endif

