#define DEBUG 0
#include "util/dprint.h"
#include "util/rwlock.h"
#include "util/xbee.h"
#include "xbee/daemon/frontend/daemon.h"
#include "xbee/daemon/frontend/resource_allocation_failed.h"
#include "xbee/daemon/frontend/robot_state.h"
#include "xbee/shared/packettypes.h"
#include <cassert>
#include <cstdlib>

namespace {
	/**
	 * Resets the shared memory block to a sensible initial state.
	 */
	void scrub_shm(pthread_rwlock_t *lck, xbeepacket::SHM_FRAME &frame) {
		rwlock_scoped_acquire acq(lck, &pthread_rwlock_wrlock);
		frame.run_data.flags = 0;
		frame.run_data.drive1_speed = 0;
		frame.run_data.drive2_speed = 0;
		frame.run_data.drive3_speed = 0;
		frame.run_data.drive4_speed = 0;
		frame.run_data.dribbler_speed = 0;
		frame.run_data.chick_power = 0;
		frame.feedback_data.flags = 0;
		frame.feedback_data.outbound_rssi = 0;
		frame.feedback_data.dribbler_speed = 0;
		frame.feedback_data.battery_level = 0;
		frame.feedback_data.faults = 0;
		frame.timestamp.tv_sec = 0;
		frame.timestamp.tv_nsec = 0;
		frame.delivery_mask = 0;
	}

	/**
	 * Stores feedback into the shared memory block as well as updating other
	 * miscellaneous fields.
	 */
	void put_feedback(pthread_rwlock_t *lck, xbeepacket::SHM_FRAME &frame, const xbeepacket::FEEDBACK_DATA &packet, const timespec &latency, uint8_t rssi) {
		rwlock_scoped_acquire acq(lck, &pthread_rwlock_wrlock);
		frame.feedback_data = packet;
		frame.delivery_mask = (frame.delivery_mask << 1) | 1;
		frame.latency = latency;
		frame.inbound_rssi = rssi;
	}
}

/**
 * A robot is in this state if it is not claimed by a client and if it has no
 * allocated resources.
 */
class robot_state::idle_state : public robot_state::state {
	public:
		static ptr enter(robot_state &bot);
		void enter_raw_mode(client *cli);
		void enter_drive_mode(client *cli);
		void release();
		void on_feedback(uint8_t rssi, const xbeepacket::FEEDBACK_DATA &packet, const timespec &latency);
		void on_feedback_timeout();
		bool claimed() const;
		bool freeing() const;
		uint16_t address16() const;
		uint8_t run_data_index() const;

	private:
		robot_state &bot;

		idle_state(robot_state &bot);
};

/**
 * A robot is in this state if it has been claimed by a client in raw mode.
 */
class robot_state::raw_state : public robot_state::state {
	public:
		static ptr enter(robot_state &bot, client &claimed_by);
		void enter_raw_mode(client *cli);
		void enter_drive_mode(client *cli);
		void release();
		void on_feedback(uint8_t rssi, const xbeepacket::FEEDBACK_DATA &packet, const timespec &latency);
		void on_feedback_timeout();
		bool claimed() const;
		bool freeing() const;
		uint16_t address16() const;
		uint8_t run_data_index() const;

	private:
		robot_state &bot;
		client &claimed_by;

		raw_state(robot_state &bot, client &claimed_by);
};

/**
 * A robot is in this state if it has been claimed by a client in drive mode but
 * the robot has not yet been given a 16-bit address.
 */
class robot_state::setting16_state : public robot_state::state {
	public:
		static ptr enter(robot_state &bot, client &claimed_by, uint16_t address16, uint8_t run_data_index);
		void enter_raw_mode(client *cli);
		void enter_drive_mode(client *cli);
		void release();
		void on_feedback(uint8_t rssi, const xbeepacket::FEEDBACK_DATA &packet, const timespec &latency);
		void on_feedback_timeout();
		bool claimed() const;
		bool freeing() const;
		uint16_t address16() const;
		uint8_t run_data_index() const;

	private:
		robot_state &bot;
		client &claimed_by;
		const uint16_t address16_;
		const uint8_t run_data_index_;

		setting16_state(robot_state &bot, client &claimed_by, uint16_t address16, uint8_t run_data_index);
		void queue_request();
		void request_done(const void *buffer, std::size_t length);
};

/**
 * A robot is in this state if it has been claimed by a client in drive mode and
 * has been given a 16-bit address but has not yet been given a run data offset.
 */
class robot_state::settingrdo_state : public robot_state::state {
	public:
		static ptr enter(robot_state &bot, client &claimed_by, uint16_t address16, uint8_t run_data_index);
		void enter_raw_mode(client *cli);
		void enter_drive_mode(client *cli);
		void release();
		void on_feedback(uint8_t rssi, const xbeepacket::FEEDBACK_DATA &packet, const timespec &latency);
		void on_feedback_timeout();
		bool claimed() const;
		bool freeing() const;
		uint16_t address16() const;
		uint8_t run_data_index() const;

	private:
		robot_state &bot;
		client &claimed_by;
		const uint16_t address16_;
		const uint8_t run_data_index_;

		settingrdo_state(robot_state &bot, client &claimed_by, uint16_t address16, uint8_t run_data_index);
		void queue_request();
		void request_done(const void *buffer, std::size_t length);
};

/**
 * A robot is in this state if it has been claimed by a client in drive mode and
 * has been given a 16-bit address and a run data offset and is therefore alive
 * and ready to drive.
 */
class robot_state::alive_state : public robot_state::state {
	public:
		static ptr enter(robot_state &bot, client &claimed_by, uint16_t address16, uint8_t run_data_index);
		void enter_raw_mode(client *cli);
		void enter_drive_mode(client *cli);
		void release();
		void on_feedback(uint8_t rssi, const xbeepacket::FEEDBACK_DATA &packet, const timespec &latency);
		void on_feedback_timeout();
		bool claimed() const;
		bool freeing() const;
		uint16_t address16() const;
		uint8_t run_data_index() const;

	private:
		robot_state &bot;
		client &claimed_by;
		const uint16_t address16_;
		const uint8_t run_data_index_;

		alive_state(robot_state &bot, client &claimed_by, uint16_t address16, uint8_t run_data_index);
};

/**
 * A robot is in this state if it has been released from drive mode and needs
 * its 16-bit address clearing.
 */
class robot_state::releasing16_state : public robot_state::state {
	public:
		static ptr enter(robot_state &bot, uint16_t address16, uint8_t run_data_index);
		void enter_raw_mode(client *cli);
		void enter_drive_mode(client *cli);
		void release();
		void on_feedback(uint8_t rssi, const xbeepacket::FEEDBACK_DATA &packet, const timespec &latency);
		void on_feedback_timeout();
		bool claimed() const;
		bool freeing() const;
		uint16_t address16() const;
		uint8_t run_data_index() const;

	private:
		robot_state &bot;
		const uint16_t address16_;
		const uint8_t run_data_index_;
		unsigned int attempts;
		static const unsigned int MAX_ATTEMPTS = 20;

		releasing16_state(robot_state &bot, uint16_t address16, uint8_t run_data_index);
		void queue_request();
		void request_done(const void *buffer, std::size_t length);
};

/**
 * A robot is in this state if it has been released from drive mode and needs
 * its bootload line to go high to reset the FPGA and clear the run data offset.
 */
class robot_state::bootloading_high_state : public robot_state::state {
	public:
		static ptr enter(robot_state &bot, uint16_t address16, uint8_t run_data_index);
		void enter_raw_mode(client *cli);
		void enter_drive_mode(client *cli);
		void release();
		void on_feedback(uint8_t rssi, const xbeepacket::FEEDBACK_DATA &packet, const timespec &latency);
		void on_feedback_timeout();
		bool claimed() const;
		bool freeing() const;
		uint16_t address16() const;
		uint8_t run_data_index() const;

	private:
		robot_state &bot;
		const uint16_t address16_;
		const uint8_t run_data_index_;
		unsigned int attempts;
		static const unsigned int MAX_ATTEMPTS = 20;

		bootloading_high_state(robot_state &bot, uint16_t address16, uint8_t run_data_index);
		void queue_request();
		void request_done(const void *buffer, std::size_t length);
};

/**
 * A robot is in this state if it has had its bootload line set high but has not
 * yet had it returned to low.
 */
class robot_state::bootloading_low_state : public robot_state::state {
	public:
		static ptr enter(robot_state &bot, uint16_t address16, uint8_t run_data_index);
		void enter_raw_mode(client *cli);
		void enter_drive_mode(client *cli);
		void release();
		void on_feedback(uint8_t rssi, const xbeepacket::FEEDBACK_DATA &packet, const timespec &latency);
		void on_feedback_timeout();
		bool claimed() const;
		bool freeing() const;
		uint16_t address16() const;
		uint8_t run_data_index() const;

	private:
		robot_state &bot;
		const uint16_t address16_;
		const uint8_t run_data_index_;
		unsigned int attempts;
		static const unsigned int MAX_ATTEMPTS = 20;

		bootloading_low_state(robot_state &bot, uint16_t address16, uint8_t run_data_index);
		void queue_request();
		void request_done(const void *buffer, std::size_t length);
};

/**
 * A robot is in this state if it has had its bootload line set high, is in the
 * process of setting it low, and received a "drive mode" claim request during
 * the time period where this statement was true, and hence must lower the
 * bootload line (in order to bring the FPGA back online) and then consider
 * itself claimed, rather than proceeding to the idle state.
 */
class robot_state::bootloading_low_to_setting16_state : public robot_state::state {
	public:
		static ptr enter(robot_state &bot, client &claimed_by, uint16_t address16, uint8_t run_data_index);
		void enter_raw_mode(client *cli);
		void enter_drive_mode(client *cli);
		void release();
		void on_feedback(uint8_t rssi, const xbeepacket::FEEDBACK_DATA &packet, const timespec &latency);
		void on_feedback_timeout();
		bool claimed() const;
		bool freeing() const;
		uint16_t address16() const;
		uint8_t run_data_index() const;

	private:
		robot_state &bot;
		client &claimed_by;
		const uint16_t address16_;
		const uint8_t run_data_index_;
		unsigned int attempts;
		static const unsigned int MAX_ATTEMPTS = 20;

		bootloading_low_to_setting16_state(robot_state &bot, client &claimed_by, uint16_t address16, uint8_t run_data_index);
		void queue_request();
		void request_done(const void *buffer, std::size_t length);
};



robot_state::state::ptr robot_state::idle_state::enter(robot_state &bot) {
	ptr p(new idle_state(bot));
	return p;
}

robot_state::idle_state::idle_state(robot_state &bot) : bot(bot) {
}

void robot_state::idle_state::enter_raw_mode(client *cli) {
	// Sanity check.
	assert(cli);

	// Transition to new state.
	bot.state_ = robot_state::raw_state::enter(bot, *cli);
}

void robot_state::idle_state::enter_drive_mode(client *cli) {
	// Sanity check.
	assert(cli);

	// Allocate resources.
	if (!bot.daemon.id16_allocator.available()) {
		throw resource_allocation_failed();
	}
	const uint8_t rdi = bot.daemon.alloc_rundata_index();
	if (rdi == 0xFF) {
		throw resource_allocation_failed();
	}
	const uint16_t address16 = bot.daemon.id16_allocator.alloc();
	assert(address16 != 0xFFFF);
	bot.daemon.run_data_index_reverse[rdi] = bot.address64;
	DPRINT(Glib::ustring::compose("Robot %1 allocated 16-bit address %2 and run data offset %3.", tohex(bot.address64, 16), tohex(address16, 4), rdi * sizeof(xbeepacket::RUN_DATA) + 1));

	// Scrub the shared memory block.
	scrub_shm(&bot.daemon.shm->lock, bot.daemon.shm->frames[rdi]);

	// Transition to new state.
	bot.state_ = robot_state::setting16_state::enter(bot, *cli, address16, rdi);
}

void robot_state::idle_state::release() {
	// This indicates confusion.
	LOG("Releasing idle robot (why?)");
}

void robot_state::idle_state::on_feedback(uint8_t, const xbeepacket::FEEDBACK_DATA &, const timespec &) {
	// Ignore this.
}

void robot_state::idle_state::on_feedback_timeout() {
	// Ignore this.
}

bool robot_state::idle_state::claimed() const {
	return false;
}

bool robot_state::idle_state::freeing() const {
	return false;
}

uint16_t robot_state::idle_state::address16() const {
	return 0;
}

uint8_t robot_state::idle_state::run_data_index() const {
	return 0xFF;
}



robot_state::state::ptr robot_state::raw_state::enter(robot_state &bot, client &claimed_by) {
	ptr p(new raw_state(bot, claimed_by));
	return p;
}

robot_state::raw_state::raw_state(robot_state &bot, client &claimed_by) : bot(bot), claimed_by(claimed_by) {
	DPRINT(Glib::ustring::compose("Robot %1 entering raw mode.", tohex(bot.address64, 16)));
}

void robot_state::raw_state::enter_raw_mode(client *) {
	LOG("Claiming a claimed robot.");
	std::abort();
}

void robot_state::raw_state::enter_drive_mode(client *) {
	LOG("Claiming a claimed robot.");
	std::abort();
}

void robot_state::raw_state::release() {
	// Transition to new state.
	DPRINT(Glib::ustring::compose("Robot %1 released from raw mode.", tohex(bot.address64, 16)));
	bot.state_ = robot_state::idle_state::enter(bot);
}

void robot_state::raw_state::on_feedback(uint8_t, const xbeepacket::FEEDBACK_DATA &, const timespec &) {
	// Ignore this.
}

void robot_state::raw_state::on_feedback_timeout() {
	// Ignore this.
}

bool robot_state::raw_state::claimed() const {
	return true;
}

bool robot_state::raw_state::freeing() const {
	return false;
}

uint16_t robot_state::raw_state::address16() const {
	return 0;
}

uint8_t robot_state::raw_state::run_data_index() const {
	return 0xFF;
}



robot_state::state::ptr robot_state::setting16_state::enter(robot_state &bot, client &claimed_by, uint16_t address16, uint8_t run_data_index) {
	ptr p(new setting16_state(bot, claimed_by, address16, run_data_index));
	return p;
}

robot_state::setting16_state::setting16_state(robot_state &bot, client &claimed_by, uint16_t address16, uint8_t run_data_index) : bot(bot), claimed_by(claimed_by), address16_(address16), run_data_index_(run_data_index) {
	queue_request();
}

void robot_state::setting16_state::enter_raw_mode(client *) {
	LOG("Claiming a claimed robot.");
	std::abort();
}

void robot_state::setting16_state::enter_drive_mode(client *) {
	LOG("Claiming a claimed robot.");
	std::abort();
}

void robot_state::setting16_state::release() {
	bot.state_ = releasing16_state::enter(bot, address16_, run_data_index_);
}

void robot_state::setting16_state::on_feedback(uint8_t, const xbeepacket::FEEDBACK_DATA &, const timespec &) {
	// Ignore this.
}

void robot_state::setting16_state::on_feedback_timeout() {
	// Ignore this.
}

bool robot_state::setting16_state::claimed() const {
	return true;
}

bool robot_state::setting16_state::freeing() const {
	return false;
}

uint16_t robot_state::setting16_state::address16() const {
	return address16_;
}

uint8_t robot_state::setting16_state::run_data_index() const {
	return run_data_index_;
}

void robot_state::setting16_state::queue_request() {
	DPRINT(Glib::ustring::compose("Queueing request to set robot %1's 16-bit address to %2.", tohex(bot.address64, 16), tohex(address16_, 4)));

	// Assemble the packet.
	xbeepacket::REMOTE_AT_REQUEST<2> packet;
	packet.apiid = xbeepacket::REMOTE_AT_REQUEST_APIID;
	packet.frame = bot.daemon.frame_number_allocator.alloc();
	xbeeutil::address_to_bytes(bot.address64, packet.address64);
	packet.address16[0] = 0xFF;
	packet.address16[1] = 0xFE;
	packet.options = xbeepacket::REMOTE_AT_REQUEST_OPTION_APPLY;
	packet.command[0] = 'M';
	packet.command[1] = 'Y';
	packet.value[0] = address16_ >> 8;
	packet.value[1] = address16_ & 0xFF;

	// Create a request object, attach a completion callback, and queue it.
	request::ptr req(request::create(&packet, sizeof(packet), true));
	req->signal_complete().connect(sigc::hide(sigc::hide(sigc::bind(sigc::mem_fun(bot.daemon.frame_number_allocator, &number_allocator<uint8_t>::free), packet.frame))));
	req->signal_complete().connect(sigc::mem_fun(this, &robot_state::setting16_state::request_done));
	bot.daemon.scheduler.queue(req);
}

void robot_state::setting16_state::request_done(const void *buffer, std::size_t length) {
	const xbeepacket::REMOTE_AT_RESPONSE &resp = *static_cast<const xbeepacket::REMOTE_AT_RESPONSE *>(buffer);

	// Check length.
	if (length < sizeof(resp)) {
		queue_request();
		return;
	}

	// Check command.
	if (resp.command[0] != 'M' || resp.command[1] != 'Y') {
		queue_request();
		return;
	}

	// Check status.
	if (resp.status == xbeepacket::REMOTE_AT_RESPONSE_STATUS_NO_RESPONSE) {
		// No response. Robot is powered down? Not an error, just try again later.
		queue_request();
	} else if (resp.status == xbeepacket::REMOTE_AT_RESPONSE_STATUS_OK) {
		// Address was assigned successfully. Next order of business is to
		// assign the run data offset.
		bot.state_ = settingrdo_state::enter(bot, claimed_by, address16_, run_data_index_);
	} else {
		// An error of some unknown type occurred. This should be impossible; it
		// suggests a logic error in the code and not a radio issue.
		LOG("A REMOTE AT RESPONSE packet has an error code that makes no sense.");
		std::abort();
	}
}



robot_state::state::ptr robot_state::settingrdo_state::enter(robot_state &bot, client &claimed_by, uint16_t address16, uint8_t run_data_index) {
	ptr p(new settingrdo_state(bot, claimed_by, address16, run_data_index));
	return p;
}

robot_state::settingrdo_state::settingrdo_state(robot_state &bot, client &claimed_by, uint16_t address16, uint8_t run_data_index) : bot(bot), claimed_by(claimed_by), address16_(address16), run_data_index_(run_data_index) {
	// Set the RUNNING flag in the run data packet, so that the scheduler will
	// start soliciting feedback. We will use the receipt of feedback as our
	// signal to exit the settingrdo state and transition to alive.
	{
		rwlock_scoped_acquire acq(&bot.daemon.shm->lock, &pthread_rwlock_wrlock);
		bot.daemon.shm->frames[run_data_index_].run_data.flags = xbeepacket::RUN_FLAG_RUNNING;
	}

	// Queue up a request.
	queue_request();
}

void robot_state::settingrdo_state::enter_raw_mode(client *) {
	LOG("Claiming a claimed robot.");
	std::abort();
}

void robot_state::settingrdo_state::enter_drive_mode(client *) {
	LOG("Claiming a claimed robot.");
	std::abort();
}

void robot_state::settingrdo_state::release() {
	bot.state_ = releasing16_state::enter(bot, address16_, run_data_index_);
}

void robot_state::settingrdo_state::on_feedback(uint8_t rssi, const xbeepacket::FEEDBACK_DATA &packet, const timespec &latency) {
	// Feedback has been received. The robot is now alive!
	DPRINT(Glib::ustring::compose("Robot %1 received feedback; becoming alive.", tohex(bot.address64, 16)));
	put_feedback(&bot.daemon.shm->lock, bot.daemon.shm->frames[run_data_index_], packet, latency, rssi);
	bot.state_ = alive_state::enter(bot, claimed_by, address16_, run_data_index_);
}

void robot_state::settingrdo_state::on_feedback_timeout() {
	// Go back and try resending the 16-bit address.
	DPRINT(Glib::ustring::compose("Robot %1 timed out on feedback; retrying 16-bit address.", tohex(bot.address64, 16)));
	bot.state_ = setting16_state::enter(bot, claimed_by, address16_, run_data_index_);
}

bool robot_state::settingrdo_state::claimed() const {
	return true;
}

bool robot_state::settingrdo_state::freeing() const {
	return false;
}

uint16_t robot_state::settingrdo_state::address16() const {
	return address16_;
}

uint8_t robot_state::settingrdo_state::run_data_index() const {
	return run_data_index_;
}

void robot_state::settingrdo_state::queue_request() {
	DPRINT(Glib::ustring::compose("Queueing request to set robot %1's run data offset to %2.", tohex(bot.address64, 16), run_data_index_ * sizeof(xbeepacket::RUN_DATA) + 1));

	// Assemble a TRANSMIT16 packet containing the run data offset.
	struct __attribute__((packed)) packet {
		xbeepacket::TRANSMIT16_HDR hdr;
		uint8_t value;
	} packet;
	packet.hdr.apiid = xbeepacket::TRANSMIT16_APIID;
	packet.hdr.frame = bot.daemon.frame_number_allocator.alloc();
	packet.hdr.address[0] = address16_ >> 8;
	packet.hdr.address[1] = address16_ & 0xFF;
	packet.hdr.options = 0;
	packet.value = run_data_index_ * sizeof(xbeepacket::RUN_DATA) + 1;

	// Create a request object, attach a completion callback, and queue it.
	request::ptr req(request::create(&packet, sizeof(packet), true));
	req->signal_complete().connect(sigc::hide(sigc::hide(sigc::bind(sigc::mem_fun(bot.daemon.frame_number_allocator, &number_allocator<uint8_t>::free), packet.hdr.frame))));
	req->signal_complete().connect(sigc::mem_fun(this, &robot_state::settingrdo_state::request_done));
	bot.daemon.scheduler.queue(req);
}

void robot_state::settingrdo_state::request_done(const void *, std::size_t) {
	// We actually don't do anything here except just retransmit the packet. We
	// keep flooding until we exit this state, which is caused by either
	// receiving feedback or timing out on feedback, and has nothing to do with
	// the RDO setting packet being delivered or not.
	queue_request();
}



robot_state::state::ptr robot_state::alive_state::enter(robot_state &bot, client &claimed_by, uint16_t address16, uint8_t run_data_index) {
	ptr p(new alive_state(bot, claimed_by, address16, run_data_index));
	return p;
}

robot_state::alive_state::alive_state(robot_state &bot, client &claimed_by, uint16_t address16, uint8_t run_data_index) : bot(bot), claimed_by(claimed_by), address16_(address16), run_data_index_(run_data_index) {
	bot.signal_alive.emit();
}

void robot_state::alive_state::enter_raw_mode(client *) {
	LOG("Claiming a claimed robot.");
	std::abort();
}

void robot_state::alive_state::enter_drive_mode(client *) {
	LOG("Claiming a claimed robot.");
	std::abort();
}

void robot_state::alive_state::release() {
	bot.state_ = releasing16_state::enter(bot, address16_, run_data_index_);
}

void robot_state::alive_state::on_feedback(uint8_t rssi, const xbeepacket::FEEDBACK_DATA &packet, const timespec &latency) {
	// Feedback has been received.
	DPRINT(Glib::ustring::compose("Robot %1 received feedback.", tohex(bot.address64, 16)));
	put_feedback(&bot.daemon.shm->lock, bot.daemon.shm->frames[run_data_index_], packet, latency, rssi);
	bot.signal_feedback.emit();
}

void robot_state::alive_state::on_feedback_timeout() {
	// Record that this happened.
	DPRINT(Glib::ustring::compose("Robot %1 timed out on feedback.", tohex(bot.address64, 16)));
	uint64_t delivery_mask;
	{
		rwlock_scoped_acquire acq(&bot.daemon.shm->lock, &pthread_rwlock_wrlock);
		bot.daemon.shm->frames[run_data_index_].delivery_mask <<= 1;
		delivery_mask = bot.daemon.shm->frames[run_data_index_].delivery_mask;
	}

	// If we have failed 64 times in a row, assume the robot is dead. Otherwise,
	// emit the feedback signal to update the UI.
	if (!delivery_mask) {
		bot.state_ = setting16_state::enter(bot, claimed_by, address16_, run_data_index_);
		bot.signal_dead.emit();
	} else {
		bot.signal_feedback.emit();
	}
}

bool robot_state::alive_state::claimed() const {
	return true;
}

bool robot_state::alive_state::freeing() const {
	return false;
}

uint16_t robot_state::alive_state::address16() const {
	return address16_;
}

uint8_t robot_state::alive_state::run_data_index() const {
	return run_data_index_;
}



robot_state::state::ptr robot_state::releasing16_state::enter(robot_state &bot, uint16_t address16, uint8_t run_data_index) {
	ptr p(new releasing16_state(bot, address16, run_data_index));
	return p;
}

robot_state::releasing16_state::releasing16_state(robot_state &bot, uint16_t address16, uint8_t run_data_index) : bot(bot), address16_(address16), run_data_index_(run_data_index), attempts(0) {
	scrub_shm(&bot.daemon.shm->lock, bot.daemon.shm->frames[run_data_index_]);
	queue_request();
}

void robot_state::releasing16_state::enter_raw_mode(client *) {
	LOG("Claiming a freeing robot in raw mode.");
	std::abort();
}

void robot_state::releasing16_state::enter_drive_mode(client *cli) {
	bot.state_ = setting16_state::enter(bot, *cli, address16_, run_data_index_);
}

void robot_state::releasing16_state::release() {
	LOG("Releasing a freeing robot (why?).");
}

void robot_state::releasing16_state::on_feedback(uint8_t, const xbeepacket::FEEDBACK_DATA &, const timespec &) {
	// Ignore this.
}

void robot_state::releasing16_state::on_feedback_timeout() {
	// Ignore this.
}

bool robot_state::releasing16_state::claimed() const {
	return false;
}

bool robot_state::releasing16_state::freeing() const {
	return true;
}

uint16_t robot_state::releasing16_state::address16() const {
	return address16_;
}

uint8_t robot_state::releasing16_state::run_data_index() const {
	return run_data_index_;
}

void robot_state::releasing16_state::queue_request() {
	DPRINT(Glib::ustring::compose("Queueing request to set robot %1's 16-bit address to FFFF.", tohex(bot.address64, 16)));

	// Assemble the packet.
	xbeepacket::REMOTE_AT_REQUEST<2> packet;
	packet.apiid = xbeepacket::REMOTE_AT_REQUEST_APIID;
	packet.frame = bot.daemon.frame_number_allocator.alloc();
	xbeeutil::address_to_bytes(bot.address64, packet.address64);
	packet.address16[0] = 0xFF;
	packet.address16[1] = 0xFE;
	packet.options = xbeepacket::REMOTE_AT_REQUEST_OPTION_APPLY;
	packet.command[0] = 'M';
	packet.command[1] = 'Y';
	packet.value[0] = 0xFF;
	packet.value[1] = 0xFF;

	// Create a request object, attach a completion callback, and queue it.
	request::ptr req(request::create(&packet, sizeof(packet), true));
	req->signal_complete().connect(sigc::hide(sigc::hide(sigc::bind(sigc::mem_fun(bot.daemon.frame_number_allocator, &number_allocator<uint8_t>::free), packet.frame))));
	req->signal_complete().connect(sigc::mem_fun(this, &robot_state::releasing16_state::request_done));
	bot.daemon.scheduler.queue(req);
}

void robot_state::releasing16_state::request_done(const void *buffer, std::size_t length) {
	const xbeepacket::REMOTE_AT_RESPONSE &resp = *static_cast<const xbeepacket::REMOTE_AT_RESPONSE *>(buffer);

	// Check length.
	if (length < sizeof(resp)) {
		queue_request();
		return;
	}

	// Check command.
	if (resp.command[0] != 'M' || resp.command[1] != 'Y') {
		queue_request();
		return;
	}

	// Check status.
	if (resp.status == xbeepacket::REMOTE_AT_RESPONSE_STATUS_NO_RESPONSE) {
		// No response. Robot is powered down? Not an error, just try again up
		// to a maximum limit of attempts.
		if (++attempts < MAX_ATTEMPTS) {
			queue_request();
			return;
		}
	} else if (resp.status == xbeepacket::REMOTE_AT_RESPONSE_STATUS_OK) {
		// Continue below.
	} else {
		// An error of some unknown type occurred. This should be impossible; it
		// suggests a logic error in the code and not a radio issue.
		LOG("A REMOTE AT RESPONSE packet has an error code that makes no sense.");
		std::abort();
	}

	// Address was assigned successfully. Next order of business is to reset the
	// FPGA.
	bot.state_ = bootloading_high_state::enter(bot, address16_, run_data_index_);
}



robot_state::state::ptr robot_state::bootloading_high_state::enter(robot_state &bot, uint16_t address16, uint8_t run_data_index) {
	ptr p(new bootloading_high_state(bot, address16, run_data_index));
	return p;
}

robot_state::bootloading_high_state::bootloading_high_state(robot_state &bot, uint16_t address16, uint8_t run_data_index) : bot(bot), address16_(address16), run_data_index_(run_data_index), attempts(0) {
	queue_request();
}

void robot_state::bootloading_high_state::enter_raw_mode(client *) {
	LOG("Claiming a freeing robot in raw mode.");
	std::abort();
}

void robot_state::bootloading_high_state::enter_drive_mode(client *cli) {
	bot.state_ = bootloading_low_to_setting16_state::enter(bot, *cli, address16_, run_data_index_);
}

void robot_state::bootloading_high_state::release() {
	LOG("Releasing a freeing robot (why?).");
}

void robot_state::bootloading_high_state::on_feedback(uint8_t, const xbeepacket::FEEDBACK_DATA &, const timespec &) {
	// Ignore this.
}

void robot_state::bootloading_high_state::on_feedback_timeout() {
	// Ignore this.
}

bool robot_state::bootloading_high_state::claimed() const {
	return false;
}

bool robot_state::bootloading_high_state::freeing() const {
	return true;
}

uint16_t robot_state::bootloading_high_state::address16() const {
	return address16_;
}

uint8_t robot_state::bootloading_high_state::run_data_index() const {
	return run_data_index_;
}

void robot_state::bootloading_high_state::queue_request() {
	DPRINT(Glib::ustring::compose("Queueing request to set robot %1 to bootload mode.", tohex(bot.address64, 16)));

	// Assemble the packet.
	xbeepacket::REMOTE_AT_REQUEST<1> packet;
	packet.apiid = xbeepacket::REMOTE_AT_REQUEST_APIID;
	packet.frame = bot.daemon.frame_number_allocator.alloc();
	xbeeutil::address_to_bytes(bot.address64, packet.address64);
	packet.address16[0] = 0xFF;
	packet.address16[1] = 0xFE;
	packet.options = xbeepacket::REMOTE_AT_REQUEST_OPTION_APPLY;
	packet.command[0] = 'D';
	packet.command[1] = '0';
	packet.value[0] = 5;

	// Create a request object, attach a completion callback, and queue it.
	request::ptr req(request::create(&packet, sizeof(packet), true));
	req->signal_complete().connect(sigc::hide(sigc::hide(sigc::bind(sigc::mem_fun(bot.daemon.frame_number_allocator, &number_allocator<uint8_t>::free), packet.frame))));
	req->signal_complete().connect(sigc::mem_fun(this, &robot_state::bootloading_high_state::request_done));
	bot.daemon.scheduler.queue(req);
}

void robot_state::bootloading_high_state::request_done(const void *buffer, std::size_t length) {
	const xbeepacket::REMOTE_AT_RESPONSE &resp = *static_cast<const xbeepacket::REMOTE_AT_RESPONSE *>(buffer);

	// Check length.
	if (length < sizeof(resp)) {
		queue_request();
		return;
	}

	// Check command.
	if (resp.command[0] != 'D' || resp.command[1] != '0') {
		queue_request();
		return;
	}

	// Check status.
	if (resp.status == xbeepacket::REMOTE_AT_RESPONSE_STATUS_NO_RESPONSE) {
		// No response. Robot is powered down? Not an error, just try again up
		// to a maximum limit of attempts.
		if (++attempts < MAX_ATTEMPTS) {
			queue_request();
			return;
		}
	} else if (resp.status == xbeepacket::REMOTE_AT_RESPONSE_STATUS_OK) {
		// Continue below.
	} else {
		// An error of some unknown type occurred. This should be impossible; it
		// suggests a logic error in the code and not a radio issue.
		LOG("A REMOTE AT RESPONSE packet has an error code that makes no sense.");
		std::abort();
	}

	// Bootload line was raised. Next order of business is to lower it again.
	bot.state_ = bootloading_low_state::enter(bot, address16_, run_data_index_);
}



robot_state::state::ptr robot_state::bootloading_low_state::enter(robot_state &bot, uint16_t address16, uint8_t run_data_index) {
	ptr p(new bootloading_low_state(bot, address16, run_data_index));
	return p;
}

robot_state::bootloading_low_state::bootloading_low_state(robot_state &bot, uint16_t address16, uint8_t run_data_index) : bot(bot), address16_(address16), run_data_index_(run_data_index), attempts(0) {
	queue_request();
}

void robot_state::bootloading_low_state::enter_raw_mode(client *) {
	LOG("Claiming a freeing robot in raw mode.");
	std::abort();
}

void robot_state::bootloading_low_state::enter_drive_mode(client *cli) {
	bot.state_ = setting16_state::enter(bot, *cli, address16_, run_data_index_);
}

void robot_state::bootloading_low_state::release() {
	LOG("Releasing a freeing robot (why?).");
}

void robot_state::bootloading_low_state::on_feedback(uint8_t, const xbeepacket::FEEDBACK_DATA &, const timespec &) {
	// Ignore this.
}

void robot_state::bootloading_low_state::on_feedback_timeout() {
	// Ignore this.
}

bool robot_state::bootloading_low_state::claimed() const {
	return false;
}

bool robot_state::bootloading_low_state::freeing() const {
	return true;
}

uint16_t robot_state::bootloading_low_state::address16() const {
	return address16_;
}

uint8_t robot_state::bootloading_low_state::run_data_index() const {
	return run_data_index_;
}

void robot_state::bootloading_low_state::queue_request() {
	DPRINT(Glib::ustring::compose("Queueing request to set robot %1 out of bootload mode.", tohex(bot.address64, 16)));

	// Assemble the packet.
	xbeepacket::REMOTE_AT_REQUEST<1> packet;
	packet.apiid = xbeepacket::REMOTE_AT_REQUEST_APIID;
	packet.frame = bot.daemon.frame_number_allocator.alloc();
	xbeeutil::address_to_bytes(bot.address64, packet.address64);
	packet.address16[0] = 0xFF;
	packet.address16[1] = 0xFE;
	packet.options = xbeepacket::REMOTE_AT_REQUEST_OPTION_APPLY;
	packet.command[0] = 'D';
	packet.command[1] = '0';
	packet.value[0] = 4;

	// Create a request object, attach a completion callback, and queue it.
	request::ptr req(request::create(&packet, sizeof(packet), true));
	req->signal_complete().connect(sigc::hide(sigc::hide(sigc::bind(sigc::mem_fun(bot.daemon.frame_number_allocator, &number_allocator<uint8_t>::free), packet.frame))));
	req->signal_complete().connect(sigc::mem_fun(this, &robot_state::bootloading_low_state::request_done));
	bot.daemon.scheduler.queue(req);
}

void robot_state::bootloading_low_state::request_done(const void *buffer, std::size_t length) {
	const xbeepacket::REMOTE_AT_RESPONSE &resp = *static_cast<const xbeepacket::REMOTE_AT_RESPONSE *>(buffer);

	// Check length.
	if (length < sizeof(resp)) {
		queue_request();
		return;
	}

	// Check command.
	if (resp.command[0] != 'D' || resp.command[1] != '0') {
		queue_request();
		return;
	}

	// Check status.
	if (resp.status == xbeepacket::REMOTE_AT_RESPONSE_STATUS_NO_RESPONSE) {
		// No response. Robot is powered down? Not an error, just try again up
		// to a maximum limit of attempts.
		if (++attempts < MAX_ATTEMPTS) {
			queue_request();
			return;
		}
	} else if (resp.status == xbeepacket::REMOTE_AT_RESPONSE_STATUS_OK) {
		// Continue below.
	} else {
		// An error of some unknown type occurred. This should be impossible; it
		// suggests a logic error in the code and not a radio issue.
		LOG("A REMOTE AT RESPONSE packet has an error code that makes no sense.");
		std::abort();
	}

	// Bootload line was lowered. We are now officially idle.
	bot.daemon.id16_allocator.free(address16_);
	bot.daemon.free_rundata_index(run_data_index_);
	bot.state_ = idle_state::enter(bot);
	bot.signal_resources_freed.emit();
}



robot_state::state::ptr robot_state::bootloading_low_to_setting16_state::enter(robot_state &bot, client &claimed_by, uint16_t address16, uint8_t run_data_index) {
	ptr p(new bootloading_low_to_setting16_state(bot, claimed_by, address16, run_data_index));
	return p;
}

robot_state::bootloading_low_to_setting16_state::bootloading_low_to_setting16_state(robot_state &bot, client &claimed_by, uint16_t address16, uint8_t run_data_index) : bot(bot), claimed_by(claimed_by), address16_(address16), run_data_index_(run_data_index), attempts(0) {
	queue_request();
}

void robot_state::bootloading_low_to_setting16_state::enter_raw_mode(client *) {
	LOG("Claiming a claimed robot.");
	std::abort();
}

void robot_state::bootloading_low_to_setting16_state::enter_drive_mode(client *) {
	LOG("Claiming a claimed robot.");
	std::abort();
}

void robot_state::bootloading_low_to_setting16_state::release() {
	bot.state_ = bootloading_low_state::enter(bot, address16_, run_data_index_);
}

void robot_state::bootloading_low_to_setting16_state::on_feedback(uint8_t, const xbeepacket::FEEDBACK_DATA &, const timespec &) {
	// Ignore this.
}

void robot_state::bootloading_low_to_setting16_state::on_feedback_timeout() {
	// Ignore this.
}

bool robot_state::bootloading_low_to_setting16_state::claimed() const {
	return true;
}

bool robot_state::bootloading_low_to_setting16_state::freeing() const {
	return false;
}

uint16_t robot_state::bootloading_low_to_setting16_state::address16() const {
	return address16_;
}

uint8_t robot_state::bootloading_low_to_setting16_state::run_data_index() const {
	return run_data_index_;
}

void robot_state::bootloading_low_to_setting16_state::queue_request() {
	DPRINT(Glib::ustring::compose("Queueing request to set robot %1 out of bootload mode while claiming.", tohex(bot.address64, 16)));

	// Assemble the packet.
	xbeepacket::REMOTE_AT_REQUEST<1> packet;
	packet.apiid = xbeepacket::REMOTE_AT_REQUEST_APIID;
	packet.frame = bot.daemon.frame_number_allocator.alloc();
	xbeeutil::address_to_bytes(bot.address64, packet.address64);
	packet.address16[0] = 0xFF;
	packet.address16[1] = 0xFE;
	packet.options = xbeepacket::REMOTE_AT_REQUEST_OPTION_APPLY;
	packet.command[0] = 'D';
	packet.command[1] = '0';
	packet.value[0] = 4;

	// Create a request object, attach a completion callback, and queue it.
	request::ptr req(request::create(&packet, sizeof(packet), true));
	req->signal_complete().connect(sigc::hide(sigc::hide(sigc::bind(sigc::mem_fun(bot.daemon.frame_number_allocator, &number_allocator<uint8_t>::free), packet.frame))));
	req->signal_complete().connect(sigc::mem_fun(this, &robot_state::bootloading_low_to_setting16_state::request_done));
	bot.daemon.scheduler.queue(req);
}

void robot_state::bootloading_low_to_setting16_state::request_done(const void *buffer, std::size_t length) {
	const xbeepacket::REMOTE_AT_RESPONSE &resp = *static_cast<const xbeepacket::REMOTE_AT_RESPONSE *>(buffer);

	// Check length.
	if (length < sizeof(resp)) {
		queue_request();
		return;
	}

	// Check command.
	if (resp.command[0] != 'D' || resp.command[1] != '0') {
		queue_request();
		return;
	}

	// Check status.
	if (resp.status == xbeepacket::REMOTE_AT_RESPONSE_STATUS_NO_RESPONSE) {
		// No response. Robot is powered down? Not an error, just try again up
		// to a maximum limit of attempts.
		if (++attempts < MAX_ATTEMPTS) {
			queue_request();
			return;
		}
	} else if (resp.status == xbeepacket::REMOTE_AT_RESPONSE_STATUS_OK) {
		// Continue below.
	} else {
		// An error of some unknown type occurred. This should be impossible; it
		// suggests a logic error in the code and not a radio issue.
		LOG("A REMOTE AT RESPONSE packet has an error code that makes no sense.");
		std::abort();
	}

	// Bootload line was lowered. Go back to configuring so we can drive.
	bot.state_ = setting16_state::enter(bot, claimed_by, address16_, run_data_index_);
}



robot_state::ptr robot_state::create(uint64_t address64, class daemon &daemon) {
	ptr p(new robot_state(address64, daemon));
	return p;
}

robot_state::robot_state(uint64_t address64, class daemon &daemon) : address64(address64), state_(idle_state::enter(*this)), daemon(daemon) {
}

void robot_state::enter_raw_mode(client *cli) {
	state_->enter_raw_mode(cli);
}

void robot_state::enter_drive_mode(client *cli) {
	state_->enter_drive_mode(cli);
}

void robot_state::release() {
	state_->release();
}

void robot_state::on_feedback(uint8_t rssi, const xbeepacket::FEEDBACK_DATA &packet, const timespec &latency) {
	state_->on_feedback(rssi, packet, latency);
}

void robot_state::on_feedback_timeout() {
	state_->on_feedback_timeout();
}

bool robot_state::claimed() const {
	return state_->claimed();
}

bool robot_state::freeing() const {
	return state_->freeing();
}

uint16_t robot_state::address16() const {
	return state_->address16();
}

uint8_t robot_state::run_data_index() const {
	return state_->run_data_index();
}

