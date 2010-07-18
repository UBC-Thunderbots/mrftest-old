#ifndef FIRMWARE_WATCHABLE_OPERATION_H
#define FIRMWARE_WATCHABLE_OPERATION_H

#include "util/noncopyable.h"
#include <glibmm.h>

//
// A generic operation whose progress can be observed.
//
class WatchableOperation : public NonCopyable {
	public:
		//
		// Destroys the object.
		//
		virtual ~WatchableOperation() {
		}

		//
		// Starts the operation.
		//
		virtual void start() = 0;

		//
		// Fired whenever progress is made.
		//
		sigc::signal<void, double> signal_progress;

		//
		// Fired when the operation completes.
		//
		sigc::signal<void> signal_finished;

		//
		// Fired when an error occurs. No further activity should occur.
		//
		sigc::signal<void, const Glib::ustring &> signal_error;

		//
		// Returns the textual status of the current operation stage.
		//
		const Glib::ustring &get_status() const {
			return status;
		}

	protected:
		//
		// The current status (should be set by subclasses).
		//
		Glib::ustring status;
};

#endif

