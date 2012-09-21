#ifndef AI_BACKEND_CLOCK_CLOCK_H
#define AI_BACKEND_CLOCK_CLOCK_H

#include <ctime>
#include <sigc++/signal.h>

namespace AI {
	namespace BE {
		namespace Clock {
			/**
			 * \brief A source of timing information, both timestamps and ticks
			 */
			class Clock {
				public:
					/**
					 * \brief Fired on each tick
					 */
					sigc::signal<void> signal_tick;

					/**
					 * \brief Returns the current time
					 *
					 * \return the current time
					 */
					virtual timespec now() const = 0;
			};
		}
	}
}

#endif

