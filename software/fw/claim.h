#ifndef FIRMWARE_CLAIM_H
#define FIRMWARE_CLAIM_H

#include "fw/watchable_operation.h"
#include "xbee/client/raw.h"

/**
 * Waits for the robot to become alive.
 */
class Claim : public WatchableOperation {
	public:
		/**
		 * Constructs a new Claim.
		 *
		 * \param bot the robot to wait for.
		 */
		Claim(RefPtr<XBeeRawBot> bot);

		/**
		 * Starts waiting for the robot to become alive.
		 */
		void start();

	private:
		const RefPtr<XBeeRawBot> bot;
		bool started, already_alive, already_failed;

		void on_alive();
		void on_failed();
};

#endif

