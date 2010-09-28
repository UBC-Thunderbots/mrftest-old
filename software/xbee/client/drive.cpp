#include "xbee/client/drive.h"
#include "util/rwlock.h"
#include "util/time.h"
#include "xbee/client/lowlevel.h"
#include "xbee/client/packet.h"
#include <cassert>
#include <cstdlib>

namespace {
	const unsigned int BATTERY_WARNING_THRESHOLD = 13500;
	const unsigned int BATTERY_NOWARNING_THRESHOLD = 14500;
	const unsigned int BATTERY_WARNING_FILTER_TIME = 10000;
	const unsigned int LT3751_FAULT_WARNING_TIME = 500;
	const unsigned int CHICKER_CHARGE_TIMEOUT = 5000;

	unsigned int smag(int val) {
		assert(-1023 <= val && val <= 1023);
		bool sign = val < 0;
		unsigned int mag = std::abs(val);
		return (sign ? 0x400 : 0x000) | mag;
	}
}

XBeeDriveBot::XBeeDriveBot(unsigned int pattern, uint64_t address, XBeeLowLevel &ll) : address(address), ll(ll), alive_(false), shm_frame(0), low_battery_message(Glib::ustring::compose("Bot %1 low battery", pattern)), lt3751_fault_message(Glib::ustring::compose("Bot %1 LT3751 fault", pattern)), chicker_low_fault_message(Glib::ustring::compose("Bot %1 chicker LOW fault", pattern)), chicker_high_fault_message(Glib::ustring::compose("Bot %1 chicker HIGH fault", pattern)), chicker_charge_timeout_message(Glib::ustring::compose("Bot %1 chicker charge timeout", pattern)) {
	timespec now;
	timespec_now(now);
	feedback_timestamp_ = now;
	low_battery_start_time = now;
	lt3751_fault_start_time = now;
	feedback_interval_.tv_sec = 0;
	feedback_interval_.tv_nsec = 0;
	run_data_interval_.tv_sec = 0;
	run_data_interval_.tv_nsec = 0;
	feedback_.flags = 0;
	feedback_.outbound_rssi = 0;
	feedback_.dribbler_speed = 0;
	feedback_.battery_level = 0;
	feedback_.faults = 0;
	ll.signal_meta.connect(sigc::mem_fun(this, &XBeeDriveBot::on_meta));
	XBeePacket::Ptr p(MetaClaimPacket::create(address, true));
	ll.send(p);
}

XBeeDriveBot::~XBeeDriveBot() {
	ll.send(MetaReleasePacket::create(address));
}

XBeeDriveBot::Ptr XBeeDriveBot::create(unsigned int pattern, uint64_t address, XBeeLowLevel &ll) {
	Ptr p(new XBeeDriveBot(pattern, address, ll));
	return p;
}

bool XBeeDriveBot::drive_faulted(unsigned int m) const {
	assert(m < 4);
	return !!(feedback_.faults & (1 << m));
}

bool XBeeDriveBot::dribbler_faulted() const {
	return !!(feedback_.faults & (1 << 4));
}

unsigned int XBeeDriveBot::battery_voltage() const {
	static const unsigned int ADC_MAX = 1023;
	static const unsigned int VCC = 3300;
	static const unsigned int DIVIDER_UPPER = 2200;
	static const unsigned int DIVIDER_LOWER = 470;
	return feedback_.battery_level * VCC / DIVIDER_LOWER * (DIVIDER_LOWER + DIVIDER_UPPER) / ADC_MAX;
}

unsigned int XBeeDriveBot::capacitor_voltage() const {
	static const unsigned int ADC_MAX = 1023;
	static const unsigned int VCC = 3300;
	static const unsigned int DIVIDER_UPPER = 220000;
	static const unsigned int DIVIDER_LOWER = 2200;
	return feedback_.capacitor_level * VCC / DIVIDER_LOWER * (DIVIDER_LOWER + DIVIDER_UPPER) / ADC_MAX;
}

unsigned int XBeeDriveBot::dribbler_speed() const {
	static const unsigned int TICKS_PER_MINUTE = 10 * 60;
	return feedback_.dribbler_speed * TICKS_PER_MINUTE;
}

int XBeeDriveBot::outbound_rssi() const {
	return -feedback_.outbound_rssi;
}

int XBeeDriveBot::inbound_rssi() const {
	return -inbound_rssi_;
}

bool XBeeDriveBot::chicker_ready() const {
	return !!(feedback_.flags & XBeePacketTypes::FEEDBACK_FLAG_CHICKER_READY);
}

bool XBeeDriveBot::lt3751_faulted() const {
	return !!(feedback_.flags & XBeePacketTypes::FEEDBACK_FLAG_CHICKER_FAULT_LT3751);
}

bool XBeeDriveBot::chicker_low_faulted() const {
	return !!(feedback_.flags & XBeePacketTypes::FEEDBACK_FLAG_CHICKER_FAULT_LOW);
}

bool XBeeDriveBot::chicker_high_faulted() const {
	return !!(feedback_.flags & XBeePacketTypes::FEEDBACK_FLAG_CHICKER_FAULT_HIGH);
}

bool XBeeDriveBot::chicker_timed_out() const {
	return !!(feedback_.flags & XBeePacketTypes::FEEDBACK_FLAG_CHICKER_CHARGE_TIMEOUT);
}

void XBeeDriveBot::stamp() {
	assert(shm_frame);
	RWLockScopedAcquire acq(&ll.shm->lock, &pthread_rwlock_rdlock);
	timespec_now(&shm_frame->timestamp);
}

void XBeeDriveBot::drive_scram() {
	assert(shm_frame);
	RWLockScopedAcquire acq(&ll.shm->lock, &pthread_rwlock_rdlock);
	shm_frame->run_data.flags &= ~(XBeePacketTypes::RUN_FLAG_DIRECT_DRIVE | XBeePacketTypes::RUN_FLAG_CONTROLLED_DRIVE);
	shm_frame->run_data.drive1_speed = 0;
	shm_frame->run_data.drive2_speed = 0;
	shm_frame->run_data.drive3_speed = 0;
	shm_frame->run_data.drive4_speed = 0;
}

void XBeeDriveBot::drive_direct(int m1, int m2, int m3, int m4) {
	assert(shm_frame);
	RWLockScopedAcquire acq(&ll.shm->lock, &pthread_rwlock_rdlock);
	shm_frame->run_data.flags &= ~XBeePacketTypes::RUN_FLAG_CONTROLLED_DRIVE;
	shm_frame->run_data.flags |= XBeePacketTypes::RUN_FLAG_DIRECT_DRIVE;
	shm_frame->run_data.drive1_speed = smag(m1);
	shm_frame->run_data.drive2_speed = smag(m2);
	shm_frame->run_data.drive3_speed = smag(m3);
	shm_frame->run_data.drive4_speed = smag(m4);
}

void XBeeDriveBot::drive_controlled(int m1, int m2, int m3, int m4) {
	assert(shm_frame);
	assert(-1023 <= m1 && m1 <= 1023);
	assert(-1023 <= m2 && m2 <= 1023);
	assert(-1023 <= m3 && m3 <= 1023);
	assert(-1023 <= m4 && m4 <= 1023);
	RWLockScopedAcquire acq(&ll.shm->lock, &pthread_rwlock_rdlock);
	shm_frame->run_data.flags &= ~XBeePacketTypes::RUN_FLAG_DIRECT_DRIVE;
	shm_frame->run_data.flags |= XBeePacketTypes::RUN_FLAG_CONTROLLED_DRIVE;
	shm_frame->run_data.drive1_speed = m1;
	shm_frame->run_data.drive2_speed = m2;
	shm_frame->run_data.drive3_speed = m3;
	shm_frame->run_data.drive4_speed = m4;
}

void XBeeDriveBot::dribble(int power) {
	assert(shm_frame);
	RWLockScopedAcquire acq(&ll.shm->lock, &pthread_rwlock_rdlock);
	shm_frame->run_data.dribbler_speed = smag(power);
}

void XBeeDriveBot::enable_chicker(bool enable) {
	assert(shm_frame);
	RWLockScopedAcquire acq(&ll.shm->lock, &pthread_rwlock_rdlock);
	if (enable) {
		shm_frame->run_data.flags |= XBeePacketTypes::RUN_FLAG_CHICKER_ENABLED;
	} else {
		shm_frame->run_data.flags &= ~XBeePacketTypes::RUN_FLAG_CHICKER_ENABLED;
	}
}

void XBeeDriveBot::kick(unsigned int width) {
	assert(shm_frame);
	assert(0 < width && width < 512);
	RWLockScopedAcquire acq(&ll.shm->lock, &pthread_rwlock_rdlock);
	shm_frame->run_data.flags &= ~XBeePacketTypes::RUN_FLAG_CHIP;
	shm_frame->run_data.chick_power = width;
	Glib::signal_timeout().connect_once(sigc::mem_fun(this, &XBeeDriveBot::clear_chick), 250);
}

void XBeeDriveBot::chip(unsigned int width) {
	assert(shm_frame);
	assert(0 < width && width < 512);
	RWLockScopedAcquire acq(&ll.shm->lock, &pthread_rwlock_rdlock);
	shm_frame->run_data.flags |= XBeePacketTypes::RUN_FLAG_CHIP;
	shm_frame->run_data.chick_power = width;
	Glib::signal_timeout().connect_once(sigc::mem_fun(this, &XBeeDriveBot::clear_chick), 250);
}

void XBeeDriveBot::on_meta(const void *buffer, std::size_t length) {
	if (length >= sizeof(XBeePacketTypes::META_HDR)) {
		uint8_t metatype = static_cast<const XBeePacketTypes::META_HDR *>(buffer)->metatype;

		if (metatype == XBeePacketTypes::CLAIM_FAILED_LOCKED_METATYPE) {
			const XBeePacketTypes::META_CLAIM_FAILED &packet = *static_cast<const XBeePacketTypes::META_CLAIM_FAILED *>(buffer);
			if (length == sizeof(packet)) {
				if (packet.address == address) {
					signal_claim_failed_locked.emit();
				}
			}
		} else if (metatype == XBeePacketTypes::CLAIM_FAILED_RESOURCE_METATYPE) {
			const XBeePacketTypes::META_CLAIM_FAILED &packet = *static_cast<const XBeePacketTypes::META_CLAIM_FAILED *>(buffer);
			if (length == sizeof(packet)) {
				if (packet.address == address) {
					signal_claim_failed_resource.emit();
				}
			}
		} else if (metatype == XBeePacketTypes::ALIVE_METATYPE) {
			const XBeePacketTypes::META_ALIVE &packet = *static_cast<const XBeePacketTypes::META_ALIVE *>(buffer);
			if (length == sizeof(packet)) {
				if (packet.address == address) {
					assert(packet.shm_frame < XBeePacketTypes::MAX_DRIVE_ROBOTS);
					shm_frame = &ll.shm->frames[packet.shm_frame];
					shm_frame->run_data.flags |= XBeePacketTypes::RUN_FLAG_RUNNING;
					alive_ = true;
					signal_alive.emit();
				}
			}
		} else if (metatype == XBeePacketTypes::DEAD_METATYPE) {
			const XBeePacketTypes::META_DEAD &packet = *static_cast<const XBeePacketTypes::META_DEAD *>(buffer);
			if (length == sizeof(packet)) {
				if (packet.address == address) {
					alive_ = false;
					signal_dead.emit();
				}
			}
		} else if (metatype == XBeePacketTypes::FEEDBACK_METATYPE) {
			const XBeePacketTypes::META_FEEDBACK &packet = *static_cast<const XBeePacketTypes::META_FEEDBACK *>(buffer);
			if (length == sizeof(packet)) {
				if (packet.address == address) {
					{
						RWLockScopedAcquire acq(&ll.shm->lock, &pthread_rwlock_rdlock);
						feedback_ = shm_frame->feedback_data;
						latency_ = shm_frame->latency;
						inbound_rssi_ = shm_frame->inbound_rssi;
						success_rate_ = __builtin_popcount(shm_frame->delivery_mask);
						run_data_interval_ = ll.shm->run_data_interval;
					}

					timespec now;
					timespec_now(now);
					timespec_sub(now, feedback_timestamp_, feedback_interval_);
					feedback_timestamp_ = now;

					if (battery_voltage() < BATTERY_WARNING_THRESHOLD) {
						timespec diff;
						timespec_sub(now, low_battery_start_time, diff);
						if (timespec_to_millis(diff) > BATTERY_WARNING_FILTER_TIME) {
							low_battery_message.activate(true);
						}
					} else {
						timespec_now(low_battery_start_time);
						if (battery_voltage() > BATTERY_NOWARNING_THRESHOLD) {
							low_battery_message.activate(false);
						}
					}

					if (lt3751_faulted()) {
						timespec diff;
						timespec_sub(now, lt3751_fault_start_time, diff);
						if (timespec_to_millis(diff) > LT3751_FAULT_WARNING_TIME) {
							lt3751_fault_message.activate(true);
						}
					} else {
						lt3751_fault_start_time = now;
						lt3751_fault_message.activate(false);
					}

					chicker_low_fault_message.activate(chicker_low_faulted());
					chicker_high_fault_message.activate(chicker_high_faulted());
					chicker_charge_timeout_message.activate(chicker_timed_out());

					signal_feedback.emit();
				}
			}
		}
	}
}

void XBeeDriveBot::clear_chick() {
	assert(shm_frame);
	RWLockScopedAcquire acq(&ll.shm->lock, &pthread_rwlock_rdlock);
	shm_frame->run_data.flags &= ~XBeePacketTypes::RUN_FLAG_CHIP;
	shm_frame->run_data.chick_power = 0;
}

