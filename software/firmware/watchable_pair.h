#ifndef FIRMWARE_WATCHABLE_PAIR_H
#define FIRMWARE_WATCHABLE_PAIR_H

#include "firmware/watchable_operation.h"

/**
 * An implementation of watchable_operation that delegates to a pair of
 * sub-operations.
 */
class watchable_pair : public watchable_operation {
	public:
		/**
		 * Constructs a new watchable_pair.
		 *
		 * \param op1 the first operation to execute.
		 * \param op2 the second operation to execute.
		 * \param weight the fraction of the progress bar that should be
		 * dedicated to the first operation.
		 */
		watchable_pair(watchable_operation &op1, watchable_operation &op2, double weight);

		/**
		 * Starts the operation. The operation will execute the first
		 * sub-operation; if this finishes successfully, the second
		 * sub-operation will be started.
		 */
		void start();

	private:
		watchable_operation &op1;
		watchable_operation &op2;
		double weight;

		void on_op1_progress(double);
		void on_op2_progress(double);
		void on_op1_finished();
};

#endif

