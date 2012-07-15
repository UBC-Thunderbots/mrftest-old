#ifndef DRIVE_DONGLE_H
#define DRIVE_DONGLE_H

#include "util/noncopyable.h"
#include "util/property.h"

namespace Drive {
	class Robot;

	/**
	 * \brief A radio dongle capable of communicating with robots
	 */
	class Dongle : public NonCopyable {
		public:
			/**
			 * \brief The possible positions of the hardware run switch
			 */
			enum class EStopState {
				/**
				 * \brief The switch is not connected properly
				 */
				BROKEN,

				/**
				 * \brief The switch is in the stop state
				 */
				STOP,

				/**
				 * \brief The switch is in the run state
				 */
				RUN,
			};

			/**
			 * \brief The current state of the emergency stop switch.
			 */
			Property<EStopState> estop_state;

			/**
			 * \brief Destroys a \code Dongle
			 */
			virtual ~Dongle();

			/**
			 * \brief Fetches an individual robot proxy
			 *
			 * \param[in] i the robot number
			 *
			 * \return the robot proxy object that allows communication with the robot
			 */
			virtual Robot &robot(unsigned int i) = 0;

		protected:
			/**
			 * \brief Constructs a new \code Dongle
			 */
			Dongle();
	};
}

#endif

