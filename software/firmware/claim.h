#ifndef FIRMWARE_CLAIM_H
#define FIRMWARE_CLAIM_H

#include "firmware/watchable_operation.h"
#include "xbee/client/raw.h"

/**
 * Waits for the robot to become alive.
 */
class claim : public watchable_operation {
	public:
		/**
		 * Constructs a new claim.
		 *
		 * \param bot the robot to wait for.
		 */
		claim(xbee_raw_bot::ptr bot);

		/**
		 * Starts waiting for the robot to become alive.
		 */
		void start();

	private:
		const xbee_raw_bot::ptr bot;
		bool started, already_alive, already_failed;

		void on_alive();
		void on_failed();
};

#endif

