#ifndef XBEE_DAEMON_FRONTEND_ROBOT_STATE_H
#define XBEE_DAEMON_FRONTEND_ROBOT_STATE_H

#include "util/byref.h"
#include "xbee/shared/packettypes.h"
#include <cstddef>
#include <ctime>
#include <glibmm.h>
#include <stdint.h>

class client;
class daemon;

//
// All the state corresponding to a single robot tracked by this arbiter.
//
class robot_state : public byref {
	public:
		//
		// A pointer to a robot_state.
		//
		typedef Glib::RefPtr<robot_state> ptr;

		//
		// The possible states a robot can be in.
		//
		enum STATE {
			//
			// Indicates that the robot is not claimed by any client and that
			// there are no resources allocated.
			//
			IDLE,

			//
			// Indicates that the robot is not claimed by any client but it has
			// resources assigned from a prior drive-mode claim that have not
			// yet been freed.
			//
			FREEING,

			//
			// Indicates that the robot is claimed in raw mode.
			//
			RAW,

			//
			// Indicates that the robot has been claimed for drive mode but is
			// not yet ready to drive. It may be powered off, it may be being
			// assigned a 16-bit address, it may be being assigned a run data
			// offset, or it may not have sent any feedback yet.
			// 
			CONFIGURING,

			//
			// Indicates that the robot is in drive mode, has been assigned its
			// resources, and that its claiming client has been notified that it
			// is alive.
			//
			ALIVE,
		};

		//
		// Constructs a new state corresponding to a dead, unclaimed robot.
		//
		static ptr create(uint64_t address64, class daemon &daemon);

		//
		// Returns the current state of the robot.
		//
		STATE state() const {
			return state_;
		}

		//
		// Returns the 16-bit address of the robot, or 0 if no address has been
		// allocated.
		//
		uint16_t address16() const {
			return address16_;
		}

		//
		// Returns the index within the run data packet where data for this
		// robot is stored.
		//
		uint8_t run_data_index() const {
			return run_data_index_;
		}

		//
		// Switches the robot into raw mode.
		//
		void enter_raw_mode(client *);

		//
		// Attempts to switch the robot into drive mode. May throw
		// resource_allocation_error if insufficient resources are available. At
		// return from this function, the state will be DRIVE_SET16.
		//
		void enter_drive_mode(client *);

		//
		// Unclaims the robot and, if it was in drive mode, starts releasing
		// resources.
		//
		void release();

		//
		// Invoked when a feedback packet is received over the radio from this
		// robot.
		//
		void on_feedback(uint8_t, const xbeepacket::FEEDBACK_DATA &, const timespec &);

		//
		// Invoked when a feedback packet was expected from this robot but was
		// not received.
		//
		void on_feedback_timeout();

		//
		// The 64-bit address of the robot.
		//
		const uint64_t address64;

		//
		// Fired when all allocated resources have been assigned and the robot is
		// alive and ready to drive.
		//
		sigc::signal<void> signal_alive;

		//
		// Fired when the robot was formerly alive but has stopped responding.
		//
		sigc::signal<void> signal_dead;

		//
		// Fired when all assigned resources have been freed during a shutdown
		// procedure.
		//
		sigc::signal<void> signal_resources_freed;

		//
		// Fired when a feedback packet is received.
		//
		sigc::signal<void> signal_feedback;

	private:
		robot_state(uint64_t address64, class daemon &daemon);
		void queue_set16();
		void set16_done(const void *, std::size_t);
		void queue_set_run_data_offset(unsigned int);
		void set_run_data_offset_done(const void *, std::size_t, unsigned int);
		void queue_bootload_high(unsigned int);
		void bootload_high_done(const void *, std::size_t, unsigned int);
		void queue_bootload_low(unsigned int);
		void bootload_low_done(const void *, std::size_t, unsigned int);
		void mark_freed();

		STATE state_;
		class daemon &daemon;
		client *claimed_by;
		uint16_t address16_;
		uint8_t run_data_index_;
		unsigned int configuring_feedback_failures;
};

#endif

