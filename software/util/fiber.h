#ifndef FIBER_H
#define FIBER_H

#include "util/byref.h"
#include <ucontext.h>
#include <vector>

class FiberGroup;

extern "C" {
	/**
	 * A private function used in implementing fibers.
	 * Application code should never call this function.
	 */
	void fiber_trampoline(int key);
}

/**
 * A cooperatively-scheduled thread of execution that can be paused at arbitrary points in time.
 *
 * A typical application will subclass this class for each distinct execution pattern (i.e. each distinct implementation of run()),
 * construct instances of those subclasses against a FiberGroup, and then use the group to run the fibers.
 */
class Fiber : public ByRef {
	public:
		/**
		 * A pointer to a fiber.
		 */
		typedef RefPtr<Fiber> Ptr;

		/**
		 * The states a fiber can be in.
		 */
		enum State {
			/**
			 * The fiber is suspended (not running), but not cancelled or finished.
			 */
			STATE_SUSPENDED,

			/**
			 * The fiber is running right now.
			 */
			STATE_RUNNING,

			/**
			 * The fiber's destructor is running and the cancellation is propagating up the fiber's stack.
			 */
			STATE_CANCELLING,

			/**
			 * The fiber's run() function has returned and the fiber is no longer executable.
			 */
			STATE_FINISHED
		};

		/**
		 * Returns the fiber's current state.
		 *
		 * \return the fiber's current state.
		 */
		State state() const {
			return state_;
		}

	protected:
		/**
		 * Constructs a new fiber.
		 *
		 * \param[in] group the group to make the fiber a member of.
		 */
		Fiber(FiberGroup &group);

		/**
		 * Destroys a fiber.
		 * This must not be called from inside FiberGroup::run() for the fiber's own group.
		 * If the fiber has not yet finished, a cancellation will be propagated up the fiber's stack.
		 */
		virtual ~Fiber();

		/**
		 * Executes the fiber's code.
		 * The application overrides this method to provide the implementation of the fiber.
		 * The function should return when the fiber should terminate.
		 */
		virtual void run() = 0;

		/**
		 * Pauses the fiber.
		 * This function can only be called from within run().
		 */
		void wait();

	private:
		const int key;
		FiberGroup &group;
		State state_;
		std::vector<char> stack;
		ucontext_t context;

		void run_impl();

		friend void fiber_trampoline(int key);
		friend class FiberGroup;
};

/**
 * A fiber group is a group of one or more fibers that all run "in lockstep".
 * An application will construct a FiberGroup then add fibers to it.
 * Once all fibers have been added, the application will then either run or tick the group to let the fibers do work.
 */
class FiberGroup : public NonCopyable {
	public:
		/**
		 * Constructs a new FiberGroup with no fibers in it.
		 */
		FiberGroup();

		/**
		 * Destroys a FiberGroup.
		 * All fibers in the group must have finished.
		 */
		~FiberGroup();

		/**
		 * Runs the fiber group to completion.
		 * This function is equivalent to calling tick() repeatedly until all fibers finish.
		 */
		void run();

		/**
		 * Runs the fiber group for one tick.
		 * The fibers will run in the order they were constructed.
		 * Each fiber will run from its current location until it calls wait() or returns from run(), at which point it will be suspended.
		 * Once all fibers have run for one tick, this function will return.
		 */
		void tick();

	private:
		std::vector<Fiber *> fibers;
		ucontext_t context;
		bool running;

		friend class Fiber;
};

#endif

