#ifndef AI_BACKEND_CLOCK_CLOCK_H
#define AI_BACKEND_CLOCK_CLOCK_H

#include "ai/common/time.h"
#include <sigc++/signal.h>

namespace AI {
	namespace BE {
		namespace Clock {
			/**
			 * \brief A source of ticks.
			 */
			class Clock {
				public:
					/**
					 * \brief Fired on each tick.
					 */
					sigc::signal<void> signal_tick;
			};
		}
	}
}

#endif

