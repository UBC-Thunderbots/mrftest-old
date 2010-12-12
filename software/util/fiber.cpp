#include "util/fiber.h"
#include "util/dprint.h"
#include "util/exception.h"
#include "util/mutex.h"
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <unordered_map>

namespace {
	/**
	 * Provides a mapping from integer keys to Fiber objects.
	 * This is necessary because pointers cannot actually be passed to makecontext().
	 */
	class Canonicalizer {
		public:
			/**
			 * Generates a new key.
			 *
			 * \return a new unique integer key that has never been seen before.
			 */
			static int key() {
				return __sync_fetch_and_add(&next_key, 1);
			}

			/**
			 * Adds a fiber to the canonicalizer.
			 *
			 * \param[in] key the key to map.
			 *
			 * \param[in] f the fiber to map to.
			 */
			static void add(int key, Fiber *f) {
				MutexScopedAcquire acq(&mutex);
				canon[key] = f;
			}

			/**
			 * Removes a fiber from the canonicalizer.
			 *
			 * \param[in] key the key to remove.
			 */
			static void remove(int key) {
				MutexScopedAcquire acq(&mutex);
				canon.erase(key);
			}

			/**
			 * Looks up the fiber for a key.
			 *
			 * \param[in] key the key to look up.
			 *
			 * \return the corresponding fiber.
			 */
			static Fiber *query(int key) {
				MutexScopedAcquire acq(&mutex);
				return canon[key];
			}

		private:
			static std::unordered_map<int, Fiber *> canon;
			static int next_key;
			static pthread_mutex_t mutex;

			Canonicalizer();
	};

	std::unordered_map<int, Fiber *> Canonicalizer::canon;
	int Canonicalizer::next_key = 0;
	pthread_mutex_t Canonicalizer::mutex = PTHREAD_MUTEX_INITIALIZER;

	/**
	 * Thrown from Fiber::wait() if a fiber is destroyed before it finishes.
	 */
	class FiberCancelledException : public std::runtime_error {
		public:
			FiberCancelledException();
			~FiberCancelledException() throw();
	};

	/**
	 * A predicate that is true if a fiber passed to it has finished.
	 */
	class FiberIsFinished {
		public:
			bool operator()(const Fiber *f) const {
				return f->state() == Fiber::STATE_FINISHED;
			}
	};
}

void fiber_trampoline(int key) {
	// The sole purpose of this function is to provide a C-compatible function for makecontext().
	// This is nothing more than a trampoline to Fiber::run_impl().
	Canonicalizer::query(key)->run_impl();
}

FiberCancelledException::FiberCancelledException() : std::runtime_error("Fiber cancelled") {
}

FiberCancelledException::~FiberCancelledException() throw() {
}

Fiber::Fiber(FiberGroup &group) : key(Canonicalizer::key()), group(group), state_(STATE_SUSPENDED), stack(1024 * 1024) {
	// Construct the new context.
	// Use the trampoline as the entry point.
	if (getcontext(&context) < 0) {
		throw SystemError("getcontext", errno);
	}
	context.uc_stack.ss_sp = &stack[0];
	context.uc_stack.ss_flags = 0;
	context.uc_stack.ss_size = stack.size();
	context.uc_link = &group.context;
	makecontext(&context, reinterpret_cast<void (*)()>(&fiber_trampoline), 1, key);

	// Register with the canonicalizer.
	Canonicalizer::add(key, this);

	// Register with the fiber group.
	group.fibers.push_back(this);
}

Fiber::~Fiber() {
	// Sanity check.
	assert(state_ != STATE_RUNNING && state_ != STATE_CANCELLING);
	assert(!group.running);

	// Check if the fiber has finished or not.
	if (state_ == STATE_SUSPENDED) {
		// The fiber is currently suspended in the middle of a run.
		// If we just destroyed it, any local variables in context when wait() was called would not have their destructors invoked.
		// This could lead to memory leaks or worse.
		// To solve this, mark the fiber as cancelled and then switch to it.
		// The wait() function, before returning, will notice that the fiber has been cancelled and throw an exception.
		// That exception will propagate up to run_impl() destroying locals as it goes.
		state_ = STATE_CANCELLING;
		if (swapcontext(&group.context, &context) < 0) {
			throw std::runtime_error("swapcontext failed");
		}
		if (state_ != STATE_FINISHED) {
			LOG_ERROR("Fiber::run() swallowed FiberCancelledException (DON'T DO THAT!)");
		}
	}

	// Unregister with the canonicalizer.
	Canonicalizer::remove(key);

	// Unregister with the fiber group.
	group.fibers.erase(std::remove(group.fibers.begin(), group.fibers.end(), this), group.fibers.end());
}

void Fiber::wait() {
	// Sanity check.
	assert(state_ == STATE_RUNNING);

	// Switch to the group's context.
	// The group will run the next fiber or return to its caller.
	state_ = STATE_SUSPENDED;
	if (swapcontext(&context, &group.context) < 0) {
		state_ = STATE_RUNNING;
		throw SystemError("swapcontext", errno);
	}

	// Now the system has switched back to us.
	// It might be that our Fiber object is being destroyed.
	// In that case, throw an exception in order to propagate up the stack and cleanly destroy local variables.
	if (state_ == STATE_CANCELLING) {
		throw FiberCancelledException();
	}

	// Sanity check.
	assert(state_ == STATE_RUNNING);
}

void Fiber::run_impl() {
	// Sanity check.
	assert(state_ == STATE_RUNNING || state_ == STATE_CANCELLING);

	// If we are already being cancelled, that's easy: just don't call run().
	if (state_ == STATE_RUNNING) {
		try {
			// Invoke the application-provided fiber function.
			run();
		} catch (const FiberCancelledException &exp) {
			// This is where the cancellation exception stops propagating.
			// Above run_impl() is C ABI, and in any case, the exception is no longer needed outside run().
			assert(state_ == STATE_CANCELLING);
		}
	}

	// Mark completion and terminate fiber.
	state_ = STATE_FINISHED;
}

FiberGroup::FiberGroup() : running(false) {
}

FiberGroup::~FiberGroup() {
	// Sanity check.
	assert(!running);
	assert(fibers.empty());
}

void FiberGroup::run() {
	while (!fibers.empty()) {
		tick();
	}
}

void FiberGroup::tick() {
	// Sanity check.
	assert(!running);

	// Mark start of tick.
	running = true;

	// Go through all the fibers.
	for (std::vector<Fiber *>::const_iterator i = fibers.begin(), iend = fibers.end(); i != iend; ++i) {
		// Switch to the fiber.
		if (swapcontext(&context, &(*i)->context) < 0) {
			running = false;
			throw SystemError("swapcontext", errno);
		}
	}

	// Erase any fibers that finished.
	fibers.erase(std::remove_if(fibers.begin(), fibers.end(), FiberIsFinished()), fibers.end());

	// Mark end of tick.
	running = false;
}

