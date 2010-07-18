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
	void scrub_shm(pthread_rwlock_t *lck, XBeePacketTypes::SHM_FRAME &frame) {
		RWLockScopedAcquire acq(lck, &pthread_rwlock_wrlock);
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
	void put_feedback(pthread_rwlock_t *lck, XBeePacketTypes::SHM_FRAME &frame, const XBeePacketTypes::FEEDBACK_DATA &packet, const timespec &latency, uint8_t rssi) {
		RWLockScopedAcquire acq(lck, &pthread_rwlock_wrlock);
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
class XBeeRobot::IdleState : public XBeeRobot::RobotState {
	public:
		static ptr enter(XBeeRobot &bot);
		void enter_raw_mode(XBeeClient *cli);
		void enter_drive_mode(XBeeClient *cli);
		void release();
		void on_feedback(uint8_t rssi, const XBeePacketTypes::FEEDBACK_DATA &packet, const timespec &latency);
		void on_feedback_timeout();
		bool claimed() const;
		bool freeing() const;
		uint16_t address16() const;
		uint8_t run_data_index() const;

	private:
		XBeeRobot &bot;

		IdleState(XBeeRobot &bot);
};

/**
 * A robot is in this state if it has been claimed by a client in raw mode.
 */
class XBeeRobot::RawState : public XBeeRobot::RobotState {
	public:
		static ptr enter(XBeeRobot &bot, XBeeClient &claimed_by, uint16_t address16);
		void enter_raw_mode(XBeeClient *cli);
		void enter_drive_mode(XBeeClient *cli);
		void release();
		void on_feedback(uint8_t rssi, const XBeePacketTypes::FEEDBACK_DATA &packet, const timespec &latency);
		void on_feedback_timeout();
		bool claimed() const;
		bool freeing() const;
		uint16_t address16() const;
		uint8_t run_data_index() const;

	private:
		XBeeRobot &bot;
		XBeeClient &claimed_by;
		const uint16_t address16_;

		RawState(XBeeRobot &bot, XBeeClient &claimed_by, uint16_t address16);
};

/**
 * A robot is in this state if it has been claimed by a client in drive mode but
 * the robot has not yet been given a 16-bit address.
 */
class XBeeRobot::Setting16State : public XBeeRobot::RobotState {
	public:
		static ptr enter(XBeeRobot &bot, XBeeClient &claimed_by, uint16_t address16, uint8_t run_data_index);
		void enter_raw_mode(XBeeClient *cli);
		void enter_drive_mode(XBeeClient *cli);
		void release();
		void on_feedback(uint8_t rssi, const XBeePacketTypes::FEEDBACK_DATA &packet, const timespec &latency);
		void on_feedback_timeout();
		bool claimed() const;
		bool freeing() const;
		uint16_t address16() const;
		uint8_t run_data_index() const;

	private:
		XBeeRobot &bot;
		XBeeClient &claimed_by;
		const uint16_t address16_;
		const uint8_t run_data_index_;

		Setting16State(XBeeRobot &bot, XBeeClient &claimed_by, uint16_t address16, uint8_t run_data_index);
		void queue_request();
		void request_done(const void *buffer, std::size_t length);
};

/**
 * A robot is in this state if it has been claimed by a client in drive mode and
 * has been given a 16-bit address but has not yet been given a run data offset.
 */
class XBeeRobot::SettingRDOState : public XBeeRobot::RobotState {
	public:
		static ptr enter(XBeeRobot &bot, XBeeClient &claimed_by, uint16_t address16, uint8_t run_data_index);
		void enter_raw_mode(XBeeClient *cli);
		void enter_drive_mode(XBeeClient *cli);
		void release();
		void on_feedback(uint8_t rssi, const XBeePacketTypes::FEEDBACK_DATA &packet, const timespec &latency);
		void on_feedback_timeout();
		bool claimed() const;
		bool freeing() const;
		uint16_t address16() const;
		uint8_t run_data_index() const;

	private:
		XBeeRobot &bot;
		XBeeClient &claimed_by;
		const uint16_t address16_;
		const uint8_t run_data_index_;

		SettingRDOState(XBeeRobot &bot, XBeeClient &claimed_by, uint16_t address16, uint8_t run_data_index);
		void queue_request();
		void request_done(const void *buffer, std::size_t length);
};

/**
 * A robot is in this state if it has been claimed by a client in drive mode and
 * has been given a 16-bit address and a run data offset and is therefore alive
 * and ready to drive.
 */
class XBeeRobot::AliveState : public XBeeRobot::RobotState {
	public:
		static ptr enter(XBeeRobot &bot, XBeeClient &claimed_by, uint16_t address16, uint8_t run_data_index);
		void enter_raw_mode(XBeeClient *cli);
		void enter_drive_mode(XBeeClient *cli);
		void release();
		void on_feedback(uint8_t rssi, const XBeePacketTypes::FEEDBACK_DATA &packet, const timespec &latency);
		void on_feedback_timeout();
		bool claimed() const;
		bool freeing() const;
		uint16_t address16() const;
		uint8_t run_data_index() const;

	private:
		XBeeRobot &bot;
		XBeeClient &claimed_by;
		const uint16_t address16_;
		const uint8_t run_data_index_;

		AliveState(XBeeRobot &bot, XBeeClient &claimed_by, uint16_t address16, uint8_t run_data_index);
};

/**
 * A robot is in this state if it has been released from drive mode and needs
 * its 16-bit address clearing.
 */
class XBeeRobot::Releasing16State : public XBeeRobot::RobotState {
	public:
		static ptr enter(XBeeRobot &bot, uint16_t address16, uint8_t run_data_index);
		void enter_raw_mode(XBeeClient *cli);
		void enter_drive_mode(XBeeClient *cli);
		void release();
		void on_feedback(uint8_t rssi, const XBeePacketTypes::FEEDBACK_DATA &packet, const timespec &latency);
		void on_feedback_timeout();
		bool claimed() const;
		bool freeing() const;
		uint16_t address16() const;
		uint8_t run_data_index() const;

	private:
		XBeeRobot &bot;
		const uint16_t address16_;
		const uint8_t run_data_index_;
		unsigned int attempts;
		static const unsigned int MAX_ATTEMPTS = 20;

		Releasing16State(XBeeRobot &bot, uint16_t address16, uint8_t run_data_index);
		void queue_request();
		void request_done(const void *buffer, std::size_t length);
};

/**
 * A robot is in this state if it has been released from drive mode and needs
 * its bootload line to go high to reset the FPGA and clear the run data offset.
 */
class XBeeRobot::BootloadingHighState : public XBeeRobot::RobotState {
	public:
		static ptr enter(XBeeRobot &bot, uint16_t address16, uint8_t run_data_index);
		void enter_raw_mode(XBeeClient *cli);
		void enter_drive_mode(XBeeClient *cli);
		void release();
		void on_feedback(uint8_t rssi, const XBeePacketTypes::FEEDBACK_DATA &packet, const timespec &latency);
		void on_feedback_timeout();
		bool claimed() const;
		bool freeing() const;
		uint16_t address16() const;
		uint8_t run_data_index() const;

	private:
		XBeeRobot &bot;
		const uint16_t address16_;
		const uint8_t run_data_index_;
		unsigned int attempts;
		static const unsigned int MAX_ATTEMPTS = 20;

		BootloadingHighState(XBeeRobot &bot, uint16_t address16, uint8_t run_data_index);
		void queue_request();
		void request_done(const void *buffer, std::size_t length);
};

/**
 * A robot is in this state if it has had its bootload line set high but has not
 * yet had it returned to low.
 */
class XBeeRobot::BootloadingLowState : public XBeeRobot::RobotState {
	public:
		static ptr enter(XBeeRobot &bot, uint16_t address16, uint8_t run_data_index);
		void enter_raw_mode(XBeeClient *cli);
		void enter_drive_mode(XBeeClient *cli);
		void release();
		void on_feedback(uint8_t rssi, const XBeePacketTypes::FEEDBACK_DATA &packet, const timespec &latency);
		void on_feedback_timeout();
		bool claimed() const;
		bool freeing() const;
		uint16_t address16() const;
		uint8_t run_data_index() const;

	private:
		XBeeRobot &bot;
		const uint16_t address16_;
		const uint8_t run_data_index_;
		unsigned int attempts;
		static const unsigned int MAX_ATTEMPTS = 20;

		BootloadingLowState(XBeeRobot &bot, uint16_t address16, uint8_t run_data_index);
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
class XBeeRobot::BootloadingLowToSetting16State : public XBeeRobot::RobotState {
	public:
		static ptr enter(XBeeRobot &bot, XBeeClient &claimed_by, uint16_t address16, uint8_t run_data_index);
		void enter_raw_mode(XBeeClient *cli);
		void enter_drive_mode(XBeeClient *cli);
		void release();
		void on_feedback(uint8_t rssi, const XBeePacketTypes::FEEDBACK_DATA &packet, const timespec &latency);
		void on_feedback_timeout();
		bool claimed() const;
		bool freeing() const;
		uint16_t address16() const;
		uint8_t run_data_index() const;

	private:
		XBeeRobot &bot;
		XBeeClient &claimed_by;
		const uint16_t address16_;
		const uint8_t run_data_index_;
		unsigned int attempts;
		static const unsigned int MAX_ATTEMPTS = 20;

		BootloadingLowToSetting16State(XBeeRobot &bot, XBeeClient &claimed_by, uint16_t address16, uint8_t run_data_index);
		void queue_request();
		void request_done(const void *buffer, std::size_t length);
};



XBeeRobot::RobotState::ptr XBeeRobot::IdleState::enter(XBeeRobot &bot) {
	ptr p(new IdleState(bot));
	return p;
}

XBeeRobot::IdleState::IdleState(XBeeRobot &bot) : bot(bot) {
}

void XBeeRobot::IdleState::enter_raw_mode(XBeeClient *cli) {
	// Sanity check.
	assert(cli);

	// Allocate resources.
	if (!bot.daemon.id16_allocator.available()) {
		throw ResourceAllocationFailed();
	}
	const uint16_t address16 = bot.daemon.id16_allocator.alloc();

	// Transition to new state.
	bot.state_ = XBeeRobot::RawState::enter(bot, *cli, address16);
}

void XBeeRobot::IdleState::enter_drive_mode(XBeeClient *cli) {
	// Sanity check.
	assert(cli);

	// Allocate resources.
	if (!bot.daemon.id16_allocator.available()) {
		throw ResourceAllocationFailed();
	}
	const uint8_t rdi = bot.daemon.alloc_rundata_index();
	if (rdi == 0xFF) {
		throw ResourceAllocationFailed();
	}
	const uint16_t address16 = bot.daemon.id16_allocator.alloc();
	assert(address16 != 0xFFFF);
	bot.daemon.run_data_index_reverse[rdi] = bot.address64;

	// Transition to new state.
	bot.state_ = XBeeRobot::Setting16State::enter(bot, *cli, address16, rdi);
}

void XBeeRobot::IdleState::release() {
	// This indicates confusion.
	LOG_WARN("Releasing idle robot (why?)");
}

void XBeeRobot::IdleState::on_feedback(uint8_t, const XBeePacketTypes::FEEDBACK_DATA &, const timespec &) {
	// Ignore this.
}

void XBeeRobot::IdleState::on_feedback_timeout() {
	// Ignore this.
}

bool XBeeRobot::IdleState::claimed() const {
	return false;
}

bool XBeeRobot::IdleState::freeing() const {
	return false;
}

uint16_t XBeeRobot::IdleState::address16() const {
	return 0;
}

uint8_t XBeeRobot::IdleState::run_data_index() const {
	return 0xFF;
}



XBeeRobot::RobotState::ptr XBeeRobot::RawState::enter(XBeeRobot &bot, XBeeClient &claimed_by, uint16_t address16) {
	ptr p(new RawState(bot, claimed_by, address16));
	return p;
}

XBeeRobot::RawState::RawState(XBeeRobot &bot, XBeeClient &claimed_by, uint16_t address16) : bot(bot), claimed_by(claimed_by), address16_(address16) {
}

void XBeeRobot::RawState::enter_raw_mode(XBeeClient *) {
	LOG_WARN("Claiming a claimed robot.");
	std::abort();
}

void XBeeRobot::RawState::enter_drive_mode(XBeeClient *) {
	LOG_WARN("Claiming a claimed robot.");
	std::abort();
}

void XBeeRobot::RawState::release() {
#warning FIGURE OUT A SENSIBLE WAY TO DECIDE WHETHER OR NOT TO FREE THE 16-BIT ADDRESS
	// Transition to new state.
	bot.state_ = XBeeRobot::IdleState::enter(bot);
}

void XBeeRobot::RawState::on_feedback(uint8_t, const XBeePacketTypes::FEEDBACK_DATA &, const timespec &) {
	// Ignore this.
}

void XBeeRobot::RawState::on_feedback_timeout() {
	// Ignore this.
}

bool XBeeRobot::RawState::claimed() const {
	return true;
}

bool XBeeRobot::RawState::freeing() const {
	return false;
}

uint16_t XBeeRobot::RawState::address16() const {
	return address16_;
}

uint8_t XBeeRobot::RawState::run_data_index() const {
	return 0xFF;
}



XBeeRobot::RobotState::ptr XBeeRobot::Setting16State::enter(XBeeRobot &bot, XBeeClient &claimed_by, uint16_t address16, uint8_t run_data_index) {
	ptr p(new Setting16State(bot, claimed_by, address16, run_data_index));
	return p;
}

XBeeRobot::Setting16State::Setting16State(XBeeRobot &bot, XBeeClient &claimed_by, uint16_t address16, uint8_t run_data_index) : bot(bot), claimed_by(claimed_by), address16_(address16), run_data_index_(run_data_index) {
	scrub_shm(&bot.daemon.shm->lock, bot.daemon.shm->frames[run_data_index]);
	queue_request();
}

void XBeeRobot::Setting16State::enter_raw_mode(XBeeClient *) {
	LOG_WARN("Claiming a claimed robot.");
	std::abort();
}

void XBeeRobot::Setting16State::enter_drive_mode(XBeeClient *) {
	LOG_WARN("Claiming a claimed robot.");
	std::abort();
}

void XBeeRobot::Setting16State::release() {
	bot.state_ = Releasing16State::enter(bot, address16_, run_data_index_);
}

void XBeeRobot::Setting16State::on_feedback(uint8_t, const XBeePacketTypes::FEEDBACK_DATA &, const timespec &) {
	// Ignore this.
}

void XBeeRobot::Setting16State::on_feedback_timeout() {
	// Ignore this.
}

bool XBeeRobot::Setting16State::claimed() const {
	return true;
}

bool XBeeRobot::Setting16State::freeing() const {
	return false;
}

uint16_t XBeeRobot::Setting16State::address16() const {
	return address16_;
}

uint8_t XBeeRobot::Setting16State::run_data_index() const {
	return run_data_index_;
}

void XBeeRobot::Setting16State::queue_request() {
	// Assemble the packet.
	XBeePacketTypes::REMOTE_AT_REQUEST<2> packet;
	packet.apiid = XBeePacketTypes::REMOTE_AT_REQUEST_APIID;
	packet.frame = bot.daemon.frame_number_allocator.alloc();
	XBeeUtil::address_to_bytes(bot.address64, packet.address64);
	packet.address16[0] = 0xFF;
	packet.address16[1] = 0xFE;
	packet.options = XBeePacketTypes::REMOTE_AT_REQUEST_OPTION_APPLY;
	packet.command[0] = 'M';
	packet.command[1] = 'Y';
	packet.value[0] = address16_ >> 8;
	packet.value[1] = address16_ & 0xFF;

	// Create a XBeeRequest object, attach a completion callback, and queue it.
	XBeeRequest::ptr req(XBeeRequest::create(&packet, sizeof(packet), true));
	req->signal_complete().connect(sigc::hide(sigc::hide(sigc::bind(sigc::mem_fun(bot.daemon.frame_number_allocator, &NumberAllocator<uint8_t>::free), packet.frame))));
	req->signal_complete().connect(sigc::mem_fun(this, &XBeeRobot::Setting16State::request_done));
	bot.daemon.scheduler.queue(req);
}

void XBeeRobot::Setting16State::request_done(const void *buffer, std::size_t length) {
	const XBeePacketTypes::REMOTE_AT_RESPONSE &resp = *static_cast<const XBeePacketTypes::REMOTE_AT_RESPONSE *>(buffer);

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
	if (resp.status == XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_NO_RESPONSE) {
		// No response. Robot is powered down? Not an error, just try again later.
		queue_request();
	} else if (resp.status == XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_OK) {
		// Address was assigned successfully. Next order of business is to
		// assign the run data offset.
		bot.state_ = SettingRDOState::enter(bot, claimed_by, address16_, run_data_index_);
	} else {
		// An error of some unknown type occurred. This should be impossible; it
		// suggests a logic error in the code and not a radio issue.
		LOG_WARN("A REMOTE AT RESPONSE packet has an error code that makes no sense.");
		std::abort();
	}
}



XBeeRobot::RobotState::ptr XBeeRobot::SettingRDOState::enter(XBeeRobot &bot, XBeeClient &claimed_by, uint16_t address16, uint8_t run_data_index) {
	ptr p(new SettingRDOState(bot, claimed_by, address16, run_data_index));
	return p;
}

XBeeRobot::SettingRDOState::SettingRDOState(XBeeRobot &bot, XBeeClient &claimed_by, uint16_t address16, uint8_t run_data_index) : bot(bot), claimed_by(claimed_by), address16_(address16), run_data_index_(run_data_index) {
	// Set the RUNNING flag in the run data packet, so that the XBeeScheduler will
	// start soliciting feedback. We will use the receipt of feedback as our
	// signal to exit the settingrdo state and transition to alive.
	{
		RWLockScopedAcquire acq(&bot.daemon.shm->lock, &pthread_rwlock_wrlock);
		bot.daemon.shm->frames[run_data_index_].run_data.flags |= XBeePacketTypes::RUN_FLAG_RUNNING;
	}

	// Queue up a XBeeRequest.
	queue_request();
}

void XBeeRobot::SettingRDOState::enter_raw_mode(XBeeClient *) {
	LOG_WARN("Claiming a claimed robot.");
	std::abort();
}

void XBeeRobot::SettingRDOState::enter_drive_mode(XBeeClient *) {
	LOG_WARN("Claiming a claimed robot.");
	std::abort();
}

void XBeeRobot::SettingRDOState::release() {
	bot.state_ = Releasing16State::enter(bot, address16_, run_data_index_);
}

void XBeeRobot::SettingRDOState::on_feedback(uint8_t rssi, const XBeePacketTypes::FEEDBACK_DATA &packet, const timespec &latency) {
	// Feedback has been received. The robot is now alive!
	put_feedback(&bot.daemon.shm->lock, bot.daemon.shm->frames[run_data_index_], packet, latency, rssi);
	bot.state_ = AliveState::enter(bot, claimed_by, address16_, run_data_index_);
}

void XBeeRobot::SettingRDOState::on_feedback_timeout() {
	// Go back and try resending the 16-bit address.
	bot.state_ = Setting16State::enter(bot, claimed_by, address16_, run_data_index_);
}

bool XBeeRobot::SettingRDOState::claimed() const {
	return true;
}

bool XBeeRobot::SettingRDOState::freeing() const {
	return false;
}

uint16_t XBeeRobot::SettingRDOState::address16() const {
	return address16_;
}

uint8_t XBeeRobot::SettingRDOState::run_data_index() const {
	return run_data_index_;
}

void XBeeRobot::SettingRDOState::queue_request() {
	// Assemble a TRANSMIT16 packet containing the run data offset.
	struct __attribute__((packed)) packet {
		XBeePacketTypes::TRANSMIT16_HDR hdr;
		uint8_t value;
	} packet;
	packet.hdr.apiid = XBeePacketTypes::TRANSMIT16_APIID;
	packet.hdr.frame = bot.daemon.frame_number_allocator.alloc();
	packet.hdr.address[0] = address16_ >> 8;
	packet.hdr.address[1] = address16_ & 0xFF;
	packet.hdr.options = 0;
	packet.value = run_data_index_ * sizeof(XBeePacketTypes::RUN_DATA) + 1;

	// Create a XBeeRequest object, attach a completion callback, and queue it.
	XBeeRequest::ptr req(XBeeRequest::create(&packet, sizeof(packet), true));
	req->signal_complete().connect(sigc::hide(sigc::hide(sigc::bind(sigc::mem_fun(bot.daemon.frame_number_allocator, &NumberAllocator<uint8_t>::free), packet.hdr.frame))));
	req->signal_complete().connect(sigc::mem_fun(this, &XBeeRobot::SettingRDOState::request_done));
	bot.daemon.scheduler.queue(req);
}

void XBeeRobot::SettingRDOState::request_done(const void *, std::size_t) {
	// We actually don't do anything here except just retransmit the packet. We
	// keep flooding until we exit this state, which is caused by either
	// receiving feedback or timing out on feedback, and has nothing to do with
	// the RDO setting packet being delivered or not.
	queue_request();
}



XBeeRobot::RobotState::ptr XBeeRobot::AliveState::enter(XBeeRobot &bot, XBeeClient &claimed_by, uint16_t address16, uint8_t run_data_index) {
	ptr p(new AliveState(bot, claimed_by, address16, run_data_index));
	return p;
}

XBeeRobot::AliveState::AliveState(XBeeRobot &bot, XBeeClient &claimed_by, uint16_t address16, uint8_t run_data_index) : bot(bot), claimed_by(claimed_by), address16_(address16), run_data_index_(run_data_index) {
	bot.signal_alive.emit();
}

void XBeeRobot::AliveState::enter_raw_mode(XBeeClient *) {
	LOG_WARN("Claiming a claimed robot.");
	std::abort();
}

void XBeeRobot::AliveState::enter_drive_mode(XBeeClient *) {
	LOG_WARN("Claiming a claimed robot.");
	std::abort();
}

void XBeeRobot::AliveState::release() {
	bot.state_ = Releasing16State::enter(bot, address16_, run_data_index_);
}

void XBeeRobot::AliveState::on_feedback(uint8_t rssi, const XBeePacketTypes::FEEDBACK_DATA &packet, const timespec &latency) {
	// Feedback has been received.
	put_feedback(&bot.daemon.shm->lock, bot.daemon.shm->frames[run_data_index_], packet, latency, rssi);
	bot.signal_feedback.emit();
}

void XBeeRobot::AliveState::on_feedback_timeout() {
	// Record that this happened.
	uint16_t delivery_mask;
	{
		RWLockScopedAcquire acq(&bot.daemon.shm->lock, &pthread_rwlock_wrlock);
		bot.daemon.shm->frames[run_data_index_].delivery_mask <<= 1;
		delivery_mask = bot.daemon.shm->frames[run_data_index_].delivery_mask;
	}

	// If we have failed 16 times in a row, assume the robot is dead. Otherwise,
	// emit the feedback signal to update the UI.
	if (!delivery_mask) {
		bot.state_ = Setting16State::enter(bot, claimed_by, address16_, run_data_index_);
		bot.signal_dead.emit();
	} else {
		bot.signal_feedback.emit();
	}
}

bool XBeeRobot::AliveState::claimed() const {
	return true;
}

bool XBeeRobot::AliveState::freeing() const {
	return false;
}

uint16_t XBeeRobot::AliveState::address16() const {
	return address16_;
}

uint8_t XBeeRobot::AliveState::run_data_index() const {
	return run_data_index_;
}



XBeeRobot::RobotState::ptr XBeeRobot::Releasing16State::enter(XBeeRobot &bot, uint16_t address16, uint8_t run_data_index) {
	ptr p(new Releasing16State(bot, address16, run_data_index));
	return p;
}

XBeeRobot::Releasing16State::Releasing16State(XBeeRobot &bot, uint16_t address16, uint8_t run_data_index) : bot(bot), address16_(address16), run_data_index_(run_data_index), attempts(0) {
	scrub_shm(&bot.daemon.shm->lock, bot.daemon.shm->frames[run_data_index_]);
	queue_request();
}

void XBeeRobot::Releasing16State::enter_raw_mode(XBeeClient *) {
	LOG_WARN("Claiming a freeing robot in raw mode.");
	std::abort();
}

void XBeeRobot::Releasing16State::enter_drive_mode(XBeeClient *cli) {
	bot.state_ = Setting16State::enter(bot, *cli, address16_, run_data_index_);
}

void XBeeRobot::Releasing16State::release() {
	LOG_WARN("Releasing a freeing robot (why?).");
}

void XBeeRobot::Releasing16State::on_feedback(uint8_t, const XBeePacketTypes::FEEDBACK_DATA &, const timespec &) {
	// Ignore this.
}

void XBeeRobot::Releasing16State::on_feedback_timeout() {
	// Ignore this.
}

bool XBeeRobot::Releasing16State::claimed() const {
	return false;
}

bool XBeeRobot::Releasing16State::freeing() const {
	return true;
}

uint16_t XBeeRobot::Releasing16State::address16() const {
	return address16_;
}

uint8_t XBeeRobot::Releasing16State::run_data_index() const {
	return run_data_index_;
}

void XBeeRobot::Releasing16State::queue_request() {
	// Assemble the packet.
	XBeePacketTypes::REMOTE_AT_REQUEST<2> packet;
	packet.apiid = XBeePacketTypes::REMOTE_AT_REQUEST_APIID;
	packet.frame = bot.daemon.frame_number_allocator.alloc();
	XBeeUtil::address_to_bytes(bot.address64, packet.address64);
	packet.address16[0] = 0xFF;
	packet.address16[1] = 0xFE;
	packet.options = XBeePacketTypes::REMOTE_AT_REQUEST_OPTION_APPLY;
	packet.command[0] = 'M';
	packet.command[1] = 'Y';
	packet.value[0] = 0xFF;
	packet.value[1] = 0xFF;

	// Create a XBeeRequest object, attach a completion callback, and queue it.
	XBeeRequest::ptr req(XBeeRequest::create(&packet, sizeof(packet), true));
	req->signal_complete().connect(sigc::hide(sigc::hide(sigc::bind(sigc::mem_fun(bot.daemon.frame_number_allocator, &NumberAllocator<uint8_t>::free), packet.frame))));
	req->signal_complete().connect(sigc::mem_fun(this, &XBeeRobot::Releasing16State::request_done));
	bot.daemon.scheduler.queue(req);
}

void XBeeRobot::Releasing16State::request_done(const void *buffer, std::size_t length) {
	const XBeePacketTypes::REMOTE_AT_RESPONSE &resp = *static_cast<const XBeePacketTypes::REMOTE_AT_RESPONSE *>(buffer);

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
	if (resp.status == XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_NO_RESPONSE) {
		// No response. Robot is powered down? Not an error, just try again up
		// to a maximum limit of attempts.
		if (++attempts < MAX_ATTEMPTS) {
			queue_request();
			return;
		}
	} else if (resp.status == XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_OK) {
		// Continue below.
	} else {
		// An error of some unknown type occurred. This should be impossible; it
		// suggests a logic error in the code and not a radio issue.
		LOG_WARN("A REMOTE AT RESPONSE packet has an error code that makes no sense.");
		std::abort();
	}

	// Address was assigned successfully. Next order of business is to reset the
	// FPGA.
	bot.state_ = BootloadingHighState::enter(bot, address16_, run_data_index_);
}



XBeeRobot::RobotState::ptr XBeeRobot::BootloadingHighState::enter(XBeeRobot &bot, uint16_t address16, uint8_t run_data_index) {
	ptr p(new BootloadingHighState(bot, address16, run_data_index));
	return p;
}

XBeeRobot::BootloadingHighState::BootloadingHighState(XBeeRobot &bot, uint16_t address16, uint8_t run_data_index) : bot(bot), address16_(address16), run_data_index_(run_data_index), attempts(0) {
	queue_request();
}

void XBeeRobot::BootloadingHighState::enter_raw_mode(XBeeClient *) {
	LOG_WARN("Claiming a freeing robot in raw mode.");
	std::abort();
}

void XBeeRobot::BootloadingHighState::enter_drive_mode(XBeeClient *cli) {
	bot.state_ = BootloadingLowToSetting16State::enter(bot, *cli, address16_, run_data_index_);
}

void XBeeRobot::BootloadingHighState::release() {
	LOG_WARN("Releasing a freeing robot (why?).");
}

void XBeeRobot::BootloadingHighState::on_feedback(uint8_t, const XBeePacketTypes::FEEDBACK_DATA &, const timespec &) {
	// Ignore this.
}

void XBeeRobot::BootloadingHighState::on_feedback_timeout() {
	// Ignore this.
}

bool XBeeRobot::BootloadingHighState::claimed() const {
	return false;
}

bool XBeeRobot::BootloadingHighState::freeing() const {
	return true;
}

uint16_t XBeeRobot::BootloadingHighState::address16() const {
	return address16_;
}

uint8_t XBeeRobot::BootloadingHighState::run_data_index() const {
	return run_data_index_;
}

void XBeeRobot::BootloadingHighState::queue_request() {
	// Assemble the packet.
	XBeePacketTypes::REMOTE_AT_REQUEST<1> packet;
	packet.apiid = XBeePacketTypes::REMOTE_AT_REQUEST_APIID;
	packet.frame = bot.daemon.frame_number_allocator.alloc();
	XBeeUtil::address_to_bytes(bot.address64, packet.address64);
	packet.address16[0] = 0xFF;
	packet.address16[1] = 0xFE;
	packet.options = XBeePacketTypes::REMOTE_AT_REQUEST_OPTION_APPLY;
	packet.command[0] = 'D';
	packet.command[1] = '0';
	packet.value[0] = 5;

	// Create a XBeeRequest object, attach a completion callback, and queue it.
	XBeeRequest::ptr req(XBeeRequest::create(&packet, sizeof(packet), true));
	req->signal_complete().connect(sigc::hide(sigc::hide(sigc::bind(sigc::mem_fun(bot.daemon.frame_number_allocator, &NumberAllocator<uint8_t>::free), packet.frame))));
	req->signal_complete().connect(sigc::mem_fun(this, &XBeeRobot::BootloadingHighState::request_done));
	bot.daemon.scheduler.queue(req);
}

void XBeeRobot::BootloadingHighState::request_done(const void *buffer, std::size_t length) {
	const XBeePacketTypes::REMOTE_AT_RESPONSE &resp = *static_cast<const XBeePacketTypes::REMOTE_AT_RESPONSE *>(buffer);

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
	if (resp.status == XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_NO_RESPONSE) {
		// No response. Robot is powered down? Not an error, just try again up
		// to a maximum limit of attempts.
		if (++attempts < MAX_ATTEMPTS) {
			queue_request();
			return;
		}
	} else if (resp.status == XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_OK) {
		// Continue below.
	} else {
		// An error of some unknown type occurred. This should be impossible; it
		// suggests a logic error in the code and not a radio issue.
		LOG_WARN("A REMOTE AT RESPONSE packet has an error code that makes no sense.");
		std::abort();
	}

	// Bootload line was raised. Next order of business is to lower it again.
	bot.state_ = BootloadingLowState::enter(bot, address16_, run_data_index_);
}



XBeeRobot::RobotState::ptr XBeeRobot::BootloadingLowState::enter(XBeeRobot &bot, uint16_t address16, uint8_t run_data_index) {
	ptr p(new BootloadingLowState(bot, address16, run_data_index));
	return p;
}

XBeeRobot::BootloadingLowState::BootloadingLowState(XBeeRobot &bot, uint16_t address16, uint8_t run_data_index) : bot(bot), address16_(address16), run_data_index_(run_data_index), attempts(0) {
	queue_request();
}

void XBeeRobot::BootloadingLowState::enter_raw_mode(XBeeClient *) {
	LOG_WARN("Claiming a freeing robot in raw mode.");
	std::abort();
}

void XBeeRobot::BootloadingLowState::enter_drive_mode(XBeeClient *cli) {
	bot.state_ = Setting16State::enter(bot, *cli, address16_, run_data_index_);
}

void XBeeRobot::BootloadingLowState::release() {
	LOG_WARN("Releasing a freeing robot (why?).");
}

void XBeeRobot::BootloadingLowState::on_feedback(uint8_t, const XBeePacketTypes::FEEDBACK_DATA &, const timespec &) {
	// Ignore this.
}

void XBeeRobot::BootloadingLowState::on_feedback_timeout() {
	// Ignore this.
}

bool XBeeRobot::BootloadingLowState::claimed() const {
	return false;
}

bool XBeeRobot::BootloadingLowState::freeing() const {
	return true;
}

uint16_t XBeeRobot::BootloadingLowState::address16() const {
	return address16_;
}

uint8_t XBeeRobot::BootloadingLowState::run_data_index() const {
	return run_data_index_;
}

void XBeeRobot::BootloadingLowState::queue_request() {
	// Assemble the packet.
	XBeePacketTypes::REMOTE_AT_REQUEST<1> packet;
	packet.apiid = XBeePacketTypes::REMOTE_AT_REQUEST_APIID;
	packet.frame = bot.daemon.frame_number_allocator.alloc();
	XBeeUtil::address_to_bytes(bot.address64, packet.address64);
	packet.address16[0] = 0xFF;
	packet.address16[1] = 0xFE;
	packet.options = XBeePacketTypes::REMOTE_AT_REQUEST_OPTION_APPLY;
	packet.command[0] = 'D';
	packet.command[1] = '0';
	packet.value[0] = 4;

	// Create a XBeeRequest object, attach a completion callback, and queue it.
	XBeeRequest::ptr req(XBeeRequest::create(&packet, sizeof(packet), true));
	req->signal_complete().connect(sigc::hide(sigc::hide(sigc::bind(sigc::mem_fun(bot.daemon.frame_number_allocator, &NumberAllocator<uint8_t>::free), packet.frame))));
	req->signal_complete().connect(sigc::mem_fun(this, &XBeeRobot::BootloadingLowState::request_done));
	bot.daemon.scheduler.queue(req);
}

void XBeeRobot::BootloadingLowState::request_done(const void *buffer, std::size_t length) {
	const XBeePacketTypes::REMOTE_AT_RESPONSE &resp = *static_cast<const XBeePacketTypes::REMOTE_AT_RESPONSE *>(buffer);

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
	if (resp.status == XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_NO_RESPONSE) {
		// No response. Robot is powered down? Not an error, just try again up
		// to a maximum limit of attempts.
		if (++attempts < MAX_ATTEMPTS) {
			queue_request();
			return;
		}
	} else if (resp.status == XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_OK) {
		// Continue below.
	} else {
		// An error of some unknown type occurred. This should be impossible; it
		// suggests a logic error in the code and not a radio issue.
		LOG_WARN("A REMOTE AT RESPONSE packet has an error code that makes no sense.");
		std::abort();
	}

	// Bootload line was lowered. We are now officially idle.
	bot.daemon.id16_allocator.free(address16_);
	bot.daemon.free_rundata_index(run_data_index_);
	bot.state_ = IdleState::enter(bot);
	bot.signal_resources_freed.emit();
}



XBeeRobot::RobotState::ptr XBeeRobot::BootloadingLowToSetting16State::enter(XBeeRobot &bot, XBeeClient &claimed_by, uint16_t address16, uint8_t run_data_index) {
	ptr p(new BootloadingLowToSetting16State(bot, claimed_by, address16, run_data_index));
	return p;
}

XBeeRobot::BootloadingLowToSetting16State::BootloadingLowToSetting16State(XBeeRobot &bot, XBeeClient &claimed_by, uint16_t address16, uint8_t run_data_index) : bot(bot), claimed_by(claimed_by), address16_(address16), run_data_index_(run_data_index), attempts(0) {
	queue_request();
}

void XBeeRobot::BootloadingLowToSetting16State::enter_raw_mode(XBeeClient *) {
	LOG_WARN("Claiming a claimed robot.");
	std::abort();
}

void XBeeRobot::BootloadingLowToSetting16State::enter_drive_mode(XBeeClient *) {
	LOG_WARN("Claiming a claimed robot.");
	std::abort();
}

void XBeeRobot::BootloadingLowToSetting16State::release() {
	bot.state_ = BootloadingLowState::enter(bot, address16_, run_data_index_);
}

void XBeeRobot::BootloadingLowToSetting16State::on_feedback(uint8_t, const XBeePacketTypes::FEEDBACK_DATA &, const timespec &) {
	// Ignore this.
}

void XBeeRobot::BootloadingLowToSetting16State::on_feedback_timeout() {
	// Ignore this.
}

bool XBeeRobot::BootloadingLowToSetting16State::claimed() const {
	return true;
}

bool XBeeRobot::BootloadingLowToSetting16State::freeing() const {
	return false;
}

uint16_t XBeeRobot::BootloadingLowToSetting16State::address16() const {
	return address16_;
}

uint8_t XBeeRobot::BootloadingLowToSetting16State::run_data_index() const {
	return run_data_index_;
}

void XBeeRobot::BootloadingLowToSetting16State::queue_request() {
	// Assemble the packet.
	XBeePacketTypes::REMOTE_AT_REQUEST<1> packet;
	packet.apiid = XBeePacketTypes::REMOTE_AT_REQUEST_APIID;
	packet.frame = bot.daemon.frame_number_allocator.alloc();
	XBeeUtil::address_to_bytes(bot.address64, packet.address64);
	packet.address16[0] = 0xFF;
	packet.address16[1] = 0xFE;
	packet.options = XBeePacketTypes::REMOTE_AT_REQUEST_OPTION_APPLY;
	packet.command[0] = 'D';
	packet.command[1] = '0';
	packet.value[0] = 4;

	// Create a XBeeRequest object, attach a completion callback, and queue it.
	XBeeRequest::ptr req(XBeeRequest::create(&packet, sizeof(packet), true));
	req->signal_complete().connect(sigc::hide(sigc::hide(sigc::bind(sigc::mem_fun(bot.daemon.frame_number_allocator, &NumberAllocator<uint8_t>::free), packet.frame))));
	req->signal_complete().connect(sigc::mem_fun(this, &XBeeRobot::BootloadingLowToSetting16State::request_done));
	bot.daemon.scheduler.queue(req);
}

void XBeeRobot::BootloadingLowToSetting16State::request_done(const void *buffer, std::size_t length) {
	const XBeePacketTypes::REMOTE_AT_RESPONSE &resp = *static_cast<const XBeePacketTypes::REMOTE_AT_RESPONSE *>(buffer);

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
	if (resp.status == XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_NO_RESPONSE) {
		// No response. Robot is powered down? Not an error, just try again up
		// to a maximum limit of attempts.
		if (++attempts < MAX_ATTEMPTS) {
			queue_request();
			return;
		}
	} else if (resp.status == XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_OK) {
		// Continue below.
	} else {
		// An error of some unknown type occurred. This should be impossible; it
		// suggests a logic error in the code and not a radio issue.
		LOG_WARN("A REMOTE AT RESPONSE packet has an error code that makes no sense.");
		std::abort();
	}

	// Bootload line was lowered. Go back to configuring so we can drive.
	bot.state_ = Setting16State::enter(bot, claimed_by, address16_, run_data_index_);
}



XBeeRobot::ptr XBeeRobot::create(uint64_t address64, class XBeeDaemon &daemon) {
	ptr p(new XBeeRobot(address64, daemon));
	return p;
}

XBeeRobot::XBeeRobot(uint64_t address64, class XBeeDaemon &daemon) : address64(address64), state_(IdleState::enter(*this)), daemon(daemon) {
}

void XBeeRobot::enter_raw_mode(XBeeClient *cli) {
	state_->enter_raw_mode(cli);
}

void XBeeRobot::enter_drive_mode(XBeeClient *cli) {
	state_->enter_drive_mode(cli);
}

void XBeeRobot::release() {
	state_->release();
}

void XBeeRobot::on_feedback(uint8_t rssi, const XBeePacketTypes::FEEDBACK_DATA &packet, const timespec &latency) {
	state_->on_feedback(rssi, packet, latency);
}

void XBeeRobot::on_feedback_timeout() {
	state_->on_feedback_timeout();
}

bool XBeeRobot::claimed() const {
	return state_->claimed();
}

bool XBeeRobot::freeing() const {
	return state_->freeing();
}

uint16_t XBeeRobot::address16() const {
	return state_->address16();
}

uint8_t XBeeRobot::run_data_index() const {
	return state_->run_data_index();
}

