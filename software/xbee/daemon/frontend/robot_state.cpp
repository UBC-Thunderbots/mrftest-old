#define DEBUG 0
#include "util/dprint.h"
#include "util/rwlock.h"
#include "util/xbee.h"
#include "xbee/daemon/frontend/daemon.h"
#include "xbee/daemon/frontend/resource_allocation_failed.h"
#include "xbee/daemon/frontend/robot_state.h"
#include "xbee/shared/packettypes.h"
#include <cassert>

namespace {
	//
	// Although there are XBee-layer acknowledgements indicating whether radio
	// delivery was successful or not, there is no acknowledgement protocol that
	// would handle an error on the robot's serial line from the XBee to the
	// FPGA. An error there would result in the run data offset value being
	// silently lost, while the robot's XBee would ACK the packet and the host
	// XBee would send a Transmit Status packet indicating success. To avoid
	// this problem, we just require successful XBee-level delivery of the run
	// data offset value more than once, under the assumption that at least one
	// of those deliveries will traverse the serial line intact and reach the
	// FPGA (the serial line is amazingly reliable, so this is a very reasonable
	// assumption).
	//
	// This is the number of transmission that are made.
	//
	const unsigned int SET_RUN_DATA_OFFSET_COUNT = 3;

	//
	// This is the maximum number of times to try sending a message while
	// deassigning resources from a released robot before giving up and assuming
	// the robot has been powered down and hence forgotten all its resources
	// anyway.
	//
	const unsigned int DEASSIGN_TRIES = 100;

	//
	// This is the maximum number of times a feedback packet can fail during the
	// CONFIGURING state before trying to resend the 16-bit address and run data
	// offset.
	//
	const unsigned int MAX_CONFIGURING_FEEDBACK_FAILURES = 5;
}

robot_state::ptr robot_state::create(uint64_t address64, class daemon &daemon) {
	robot_state::ptr p(new robot_state(address64, daemon));
	return p;
}

robot_state::robot_state(uint64_t address64, class daemon &daemon) : address64(address64), state_(robot_state::IDLE), daemon(daemon), claimed_by(0), address16_(0), run_data_index_(0xFF) {
}

void robot_state::enter_raw_mode(client *cli) {
	assert(state_ == IDLE);

	DPRINT("Robot claimed for raw mode.");

	state_ = RAW;
	claimed_by = cli;
}

void robot_state::enter_drive_mode(client *cli) {
	assert(state_ == IDLE || state_ == FREEING);

	DPRINT("Robot claimed for drive mode.");

	// Allocate a new 16-bit address if we don't already have one.
	uint16_t new_address16 = 0;
	if (!address16_) {
		if (!daemon.id16_allocator.available()) {
			throw resource_allocation_failed();
		}
		new_address16 = daemon.id16_allocator.alloc();
	}

	// Allocate a new run data offset if we don't already have one.
	uint8_t new_run_data_index = 0xFF;
	if (run_data_index_ == 0xFF) {
		new_run_data_index = daemon.alloc_rundata_index();
		if (new_run_data_index == 0xFF) {
			if (new_address16) {
				daemon.id16_allocator.free(new_address16);
			}
			throw resource_allocation_failed();
		}
	}

	// Record new state.
	state_ = robot_state::CONFIGURING;
	claimed_by = cli;
	if (new_address16) {
		address16_ = new_address16;
	}
	if (new_run_data_index != 0xFF) {
		run_data_index_ = new_run_data_index;
	}
	daemon.run_data_index_reverse[run_data_index_] = address64;

	DPRINT(Glib::ustring::compose("Using 16-bit address %1, run data frame %2", address16_, run_data_index_));

	// Scrub this robot's shared memory block.
	daemon.shm->frames[run_data_index_].run_data.flags = 0;
	daemon.shm->frames[run_data_index_].run_data.drive1_speed = 0;
	daemon.shm->frames[run_data_index_].run_data.drive2_speed = 0;
	daemon.shm->frames[run_data_index_].run_data.drive3_speed = 0;
	daemon.shm->frames[run_data_index_].run_data.drive4_speed = 0;
	daemon.shm->frames[run_data_index_].run_data.dribbler_speed = 0;
	daemon.shm->frames[run_data_index_].run_data.chick_power = 0;
	daemon.shm->frames[run_data_index_].feedback_data.flags = 0;
	daemon.shm->frames[run_data_index_].feedback_data.outbound_rssi = 0;
	daemon.shm->frames[run_data_index_].feedback_data.dribbler_speed = 0;
	daemon.shm->frames[run_data_index_].feedback_data.battery_level = 0;
	daemon.shm->frames[run_data_index_].feedback_data.faults = 0;
	daemon.shm->frames[run_data_index_].timestamp.tv_sec = 0;
	daemon.shm->frames[run_data_index_].timestamp.tv_nsec = 0;
	daemon.shm->frames[run_data_index_].delivery_mask = 0;

	// Queue a request to set the robot's 16-bit address.
	queue_set16();
}

void robot_state::queue_set16() {
	DPRINT("Queueing set-16-bit-address request.");

	// Assemble a remote AT command to send MY (set 16-bit address).
	xbeepacket::REMOTE_AT_REQUEST<2> packet;
	packet.apiid = xbeepacket::REMOTE_AT_REQUEST_APIID;
	packet.frame = daemon.frame_number_allocator.alloc();
	xbeeutil::address_to_bytes(address64, packet.address64);
	packet.address16[0] = 0xFF;
	packet.address16[1] = 0xFE;
	packet.options = xbeepacket::REMOTE_AT_REQUEST_OPTION_APPLY;
	packet.command[0] = 'M';
	packet.command[1] = 'Y';
	packet.value[0] = address16_ >> 8;
	packet.value[1] = address16_ & 0xFF;

	// Create a request object, attach a completion callback, and queue it.
	request::ptr req(request::create(&packet, sizeof(packet), true));
	req->signal_complete().connect(sigc::hide(sigc::hide(sigc::bind(sigc::mem_fun(daemon.frame_number_allocator, &number_allocator<uint8_t>::free), packet.frame))));
	req->signal_complete().connect(sigc::mem_fun(this, &robot_state::set16_done));
	daemon.scheduler.queue(req);
}

void robot_state::set16_done(const void *buffer, std::size_t length) {
	const xbeepacket::REMOTE_AT_RESPONSE &resp = *static_cast<const xbeepacket::REMOTE_AT_RESPONSE *>(buffer);

	// Check length.
	if (length < sizeof(resp)) {
		queue_set16();
		return;
	}

	// Check command.
	if (resp.command[0] != 'M' || resp.command[1] != 'Y') {
		queue_set16();
		return;
	}

	// Check status.
	if (resp.status == xbeepacket::REMOTE_AT_RESPONSE_STATUS_NO_RESPONSE) {
		// No response. Robot is powered down? Not an error, just try again later.
		queue_set16();
	} else if (resp.status == xbeepacket::REMOTE_AT_RESPONSE_STATUS_OK) {
		// Address was assigned successfully. Next order of business is to
		// assign the run data offset.
		queue_set_run_data_offset(0);
	} else {
		// An error of some unknown type occurred. This should be impossible; it
		// suggests a logic error in the code and not a radio issue.
		assert(false);
	}
}

void robot_state::queue_set_run_data_offset(unsigned int counter) {
	DPRINT("Queueing set-run-data-offset packet.");
	assert(run_data_index_ != 0xFF);

	// Assemble a TRANSMIT16 packet containing the run data offset.
	struct __attribute__((packed)) packet {
		xbeepacket::TRANSMIT16_HDR hdr;
		uint8_t value;
	} packet;
	packet.hdr.apiid = xbeepacket::TRANSMIT16_APIID;
	packet.hdr.frame = daemon.frame_number_allocator.alloc();
	packet.hdr.address[0] = address16_ >> 8;
	packet.hdr.address[1] = address16_ & 0xFF;
	packet.hdr.options = 0;
	packet.value = run_data_index_ * sizeof(xbeepacket::RUN_DATA) + 1;

	// Create a request object, attach a completion callback, and queue it.
	request::ptr req(request::create(&packet, sizeof(packet), true));
	req->signal_complete().connect(sigc::hide(sigc::hide(sigc::bind(sigc::mem_fun(daemon.frame_number_allocator, &number_allocator<uint8_t>::free), packet.hdr.frame))));
	req->signal_complete().connect(sigc::bind(sigc::mem_fun(this, &robot_state::set_run_data_offset_done), counter));
	daemon.scheduler.queue(req);
}

void robot_state::set_run_data_offset_done(const void *buffer, std::size_t length, unsigned int counter) {
	assert(run_data_index_ != 0xFF);

	const xbeepacket::TRANSMIT_STATUS &resp = *static_cast<const xbeepacket::TRANSMIT_STATUS *>(buffer);

	// Check length.
	if (length < sizeof(resp)) {
		queue_set16();
		return;
	}

	// Check status.
	if (resp.status == xbeepacket::TRANSMIT_STATUS_SUCCESS) {
		// Packet was transmitted successfully. Check if we've done this as many
		// times as we need to.
		if (counter == SET_RUN_DATA_OFFSET_COUNT) {
			// OK, we're done. Set the run flag in the shared memory packet
			// structure. This will signal to the scheduler that it should start
			// soliciting feedback packets from this robot, which will
			// eventually find their way back to robot_state::on_feedback().
			DPRINT("Run data offset assumed accepted; expecting feedback.");
			configuring_feedback_failures = 0;
			daemon.shm->frames[run_data_index_].run_data.flags = xbeepacket::RUN_FLAG_RUNNING;
		} else {
			// Need to send another copy.
			queue_set_run_data_offset(counter + 1);
		}
	} else {
		// Packet was not transmitted successfully. Start over.
		queue_set16();
	}
}

void robot_state::on_feedback(uint8_t rssi, const xbeepacket::FEEDBACK_DATA &packet, const timespec &latency) {
	DPRINT("Feedback received.");

	{
		// Take the lock.
		rwlock_scoped_acquire acq(&daemon.shm->lock, &pthread_rwlock_wrlock);

		// Save the feedback data into the shared memory block.
		daemon.shm->frames[run_data_index_].feedback_data = packet;

		// Update the feedback mask.
		daemon.shm->frames[run_data_index_].delivery_mask <<= 1;
		daemon.shm->frames[run_data_index_].delivery_mask |= UINT64_C(1);

		// Update the latency estimate.
		daemon.shm->frames[run_data_index_].latency = latency;

		// Update the inbound RSSI.
		daemon.shm->frames[run_data_index_].inbound_rssi = rssi;
	}

	// Notify the client.
	if (state_ == CONFIGURING) {
		state_ = ALIVE;
		signal_alive.emit();
	} else {
		signal_feedback.emit();
	}
}

void robot_state::on_feedback_timeout() {
	DPRINT("Timeout waiting for feedback.");

	uint64_t mask;

	{
		// Take the lock.
		rwlock_scoped_acquire acq(&daemon.shm->lock, &pthread_rwlock_wrlock);

		// Update the feedback mask and save it for later examination.
		daemon.shm->frames[run_data_index_].delivery_mask <<= 1;
		mask = daemon.shm->frames[run_data_index_].delivery_mask;
	}

	// If we have failed many feedback requests consecutively, assume the robot
	// is dead, notify the client, and go back to trying to configure.
	if (state_ == ALIVE && !mask) {
		DPRINT("64 failures; robot is dead.");

		// Record state.
		state_ = CONFIGURING;

		// Notify client.
		signal_dead.emit();

		// Scrub this robot's shared memory block.
		{
			rwlock_scoped_acquire acq(&daemon.shm->lock, &pthread_rwlock_wrlock);
			daemon.shm->frames[run_data_index_].run_data.flags = xbeepacket::RUN_FLAG_RUNNING;
			daemon.shm->frames[run_data_index_].run_data.drive1_speed = 0;
			daemon.shm->frames[run_data_index_].run_data.drive2_speed = 0;
			daemon.shm->frames[run_data_index_].run_data.drive3_speed = 0;
			daemon.shm->frames[run_data_index_].run_data.drive4_speed = 0;
			daemon.shm->frames[run_data_index_].run_data.dribbler_speed = 0;
			daemon.shm->frames[run_data_index_].run_data.chick_power = 0;
			daemon.shm->frames[run_data_index_].feedback_data.flags = 0;
			daemon.shm->frames[run_data_index_].feedback_data.outbound_rssi = 0;
			daemon.shm->frames[run_data_index_].feedback_data.dribbler_speed = 0;
			daemon.shm->frames[run_data_index_].feedback_data.battery_level = 0;
			daemon.shm->frames[run_data_index_].feedback_data.faults = 0;
			daemon.shm->frames[run_data_index_].timestamp.tv_sec = 0;
			daemon.shm->frames[run_data_index_].timestamp.tv_nsec = 0;
			daemon.shm->frames[run_data_index_].delivery_mask = 0;
		}

		// Queue a request to set the robot's 16-bit address.
		queue_set16();
		return;
	}

	// If we're configuring and see a lot of failures, go back and try assigning
	// the 16-bit address from the beginning.
	if (state_ == CONFIGURING && ++configuring_feedback_failures == MAX_CONFIGURING_FEEDBACK_FAILURES) {
		DPRINT("Failed to configure; retrying resource assignment.");

		// No need to notify or change state; this is just a retry.
		queue_set16();
		return;
	}
}

void robot_state::release() {
	DPRINT("Releasing robot.");

	// Scrub the shared memory frame, if we have one.
	if (run_data_index_ != 0xFF) {
		rwlock_scoped_acquire acq(&daemon.shm->lock, &pthread_rwlock_wrlock);
		daemon.shm->frames[run_data_index_].run_data.flags = 0;
		daemon.shm->frames[run_data_index_].run_data.drive1_speed = 0;
		daemon.shm->frames[run_data_index_].run_data.drive2_speed = 0;
		daemon.shm->frames[run_data_index_].run_data.drive3_speed = 0;
		daemon.shm->frames[run_data_index_].run_data.drive4_speed = 0;
		daemon.shm->frames[run_data_index_].run_data.dribbler_speed = 0;
		daemon.shm->frames[run_data_index_].run_data.chick_power = 0;
		daemon.shm->frames[run_data_index_].feedback_data.flags = 0;
		daemon.shm->frames[run_data_index_].feedback_data.outbound_rssi = 0;
		daemon.shm->frames[run_data_index_].feedback_data.dribbler_speed = 0;
		daemon.shm->frames[run_data_index_].feedback_data.battery_level = 0;
		daemon.shm->frames[run_data_index_].feedback_data.faults = 0;
		daemon.shm->frames[run_data_index_].timestamp.tv_sec = 0;
		daemon.shm->frames[run_data_index_].timestamp.tv_nsec = 0;
		daemon.shm->frames[run_data_index_].delivery_mask = 0;
	}

	switch (state_) {
		case IDLE:
			// Nothing to do here; we're already released.
			break;

		case FREEING:
			// Nothing to do here; we're already on the way to being idle.
			break;

		case RAW:
			// OK, just release the claim.
			state_ = IDLE;
			claimed_by = 0;
			break;

		case CONFIGURING:
		case ALIVE:
			// Need to deassign, and then later deallocate, the resources.
			state_ = FREEING;
			claimed_by = 0;
			queue_bootload_high(0);
			break;
	}
}

void robot_state::queue_bootload_high(unsigned int tries) {
	DPRINT("Queueing bootload-high request.");

	// Assemble a remote AT command to send D0 (set I/O pin bootload high).
	xbeepacket::REMOTE_AT_REQUEST<1> packet;
	packet.apiid = xbeepacket::REMOTE_AT_REQUEST_APIID;
	packet.frame = daemon.frame_number_allocator.alloc();
	xbeeutil::address_to_bytes(address64, packet.address64);
	packet.address16[0] = 0xFF;
	packet.address16[1] = 0xFE;
	packet.options = xbeepacket::REMOTE_AT_REQUEST_OPTION_APPLY;
	packet.command[0] = 'D';
	packet.command[1] = '0';
	packet.value[0] = 5;

	// Create a request object, attach a completion callback, and queue it.
	request::ptr req(request::create(&packet, sizeof(packet), true));
	req->signal_complete().connect(sigc::hide(sigc::hide(sigc::bind(sigc::mem_fun(daemon.frame_number_allocator, &number_allocator<uint8_t>::free), packet.frame))));
	req->signal_complete().connect(sigc::bind(sigc::mem_fun(this, &robot_state::bootload_high_done), tries));
	daemon.scheduler.queue(req);
}

void robot_state::bootload_high_done(const void *buffer, std::size_t length, unsigned int tries) {
	const xbeepacket::REMOTE_AT_RESPONSE &resp = *static_cast<const xbeepacket::REMOTE_AT_RESPONSE *>(buffer);

	// Check for running out of attempts.
	if (tries == DEASSIGN_TRIES) {
		queue_bootload_low(0);
		return;
	}

	// Check length.
	if (length < sizeof(resp)) {
		queue_bootload_high(tries + 1);
		return;
	}

	// Check command.
	if (resp.command[0] != 'D' || resp.command[1] != '0') {
		queue_bootload_high(tries + 1);
		return;
	}

	// Check status.
	if (resp.status == xbeepacket::REMOTE_AT_RESPONSE_STATUS_NO_RESPONSE) {
		// No response. Try again.
		queue_bootload_high(tries + 1);
	} else if (resp.status == xbeepacket::REMOTE_AT_RESPONSE_STATUS_OK) {
		// I/O line was raised successfully. Next order of business is to lower
		// the line line again.
		queue_bootload_low(0);
	} else {
		// An error of some unknown type occurred. This should be impossible; it
		// suggests a logic error in the code and not a radio issue.
		assert(false);
	}
}

void robot_state::queue_bootload_low(unsigned int tries) {
	DPRINT("Queueing bootload-low request.");

	// Assemble a remote AT command to send D0 (set I/O pin bootload low).
	xbeepacket::REMOTE_AT_REQUEST<1> packet;
	packet.apiid = xbeepacket::REMOTE_AT_REQUEST_APIID;
	packet.frame = daemon.frame_number_allocator.alloc();
	xbeeutil::address_to_bytes(address64, packet.address64);
	packet.address16[0] = 0xFF;
	packet.address16[1] = 0xFE;
	packet.options = xbeepacket::REMOTE_AT_REQUEST_OPTION_APPLY;
	packet.command[0] = 'D';
	packet.command[1] = '0';
	packet.value[0] = 4;

	// Create a request object, attach a completion callback, and queue it.
	request::ptr req(request::create(&packet, sizeof(packet), true));
	req->signal_complete().connect(sigc::hide(sigc::hide(sigc::bind(sigc::mem_fun(daemon.frame_number_allocator, &number_allocator<uint8_t>::free), packet.frame))));
	req->signal_complete().connect(sigc::bind(sigc::mem_fun(this, &robot_state::bootload_low_done), tries));
	daemon.scheduler.queue(req);
}

void robot_state::bootload_low_done(const void *buffer, std::size_t length, unsigned int tries) {
	const xbeepacket::REMOTE_AT_RESPONSE &resp = *static_cast<const xbeepacket::REMOTE_AT_RESPONSE *>(buffer);

	// Check for running out of attempts.
	if (tries == DEASSIGN_TRIES) {
		mark_freed();
		return;
	}

	// Check length.
	if (length < sizeof(resp)) {
		queue_bootload_low(tries + 1);
		return;
	}

	// Check command.
	if (resp.command[0] != 'D' || resp.command[1] != '0') {
		queue_bootload_low(tries + 1);
		return;
	}

	// Check status.
	if (resp.status == xbeepacket::REMOTE_AT_RESPONSE_STATUS_NO_RESPONSE) {
		// No response. Try again.
		queue_bootload_low(tries + 1);
	} else if (resp.status == xbeepacket::REMOTE_AT_RESPONSE_STATUS_OK) {
		// I/O line was lowered successfully.
		mark_freed();
	} else {
		// An error of some unknown type occurred. This should be impossible; it
		// suggests a logic error in the code and not a radio issue.
		assert(false);
	}
}

void robot_state::mark_freed() {
	state_ = IDLE;
	if (address16_) {
		daemon.id16_allocator.free(address16_);
		address16_ = 0;
	}
	if (run_data_index_ != 0xFF) {
		daemon.free_rundata_index(run_data_index_);
		run_data_index_ = 0xFF;
	}
	signal_resources_freed.emit();
}

