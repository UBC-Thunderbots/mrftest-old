#include "util/rwlock.h"
#include "util/time.h"
#include "xbee/client/drive.h"
#include "xbee/client/lowlevel.h"
#include "xbee/client/packet.h"
#include <cassert>
#include <cstdlib>

namespace {
	unsigned int smag(int val) {
		assert(-1023 <= val && val <= 1023);
		bool sign = val < 0;
		unsigned int mag = std::abs(val);
		return (sign ? 0x400 : 0x000) | mag;
	}
}

xbee_drive_bot::xbee_drive_bot(uint64_t address, xbee_lowlevel &ll) : address(address), ll(ll), alive_(false), shm_frame(0) {
	feedback_.flags = 0;
	feedback_.outbound_rssi = 0;
	feedback_.dribbler_speed = 0;
	feedback_.battery_level = 0;
	feedback_.faults = 0;
	ll.signal_meta.connect(sigc::mem_fun(this, &xbee_drive_bot::on_meta));
	packet::ptr p(meta_claim_packet::create(address, true));
	ll.send(p);
}

xbee_drive_bot::~xbee_drive_bot() {
	ll.send(meta_release_packet::create(address));
}

bool xbee_drive_bot::drive_faulted(unsigned int m) const {
	assert(m < 4);
	return !!(feedback_.faults & (1 << m));
}

bool xbee_drive_bot::dribbler_faulted() const {
	return !!(feedback_.faults & (1 << 4));
}

unsigned int xbee_drive_bot::battery_voltage() const {
	static const unsigned int ADC_MAX = 1023;
	static const unsigned int VCC = 3300;
	static const unsigned int DIVIDER_UPPER = 2200;
	static const unsigned int DIVIDER_LOWER = 470;
	return feedback_.battery_level * VCC / DIVIDER_LOWER * (DIVIDER_LOWER + DIVIDER_UPPER) / ADC_MAX;
}

unsigned int xbee_drive_bot::dribbler_speed() const {
	static const unsigned int TICKS_PER_MINUTE = 10 * 60;
	return feedback_.dribbler_speed * TICKS_PER_MINUTE;
}

int xbee_drive_bot::outbound_rssi() const {
	return -feedback_.outbound_rssi;
}

int xbee_drive_bot::inbound_rssi() const {
	return -inbound_rssi_;
}

bool xbee_drive_bot::chicker_ready() const {
	return !!(feedback_.flags & xbeepacket::FEEDBACK_FLAG_CHICKER_READY);
}

bool xbee_drive_bot::chicker_faulted() const {
	return !!(feedback_.flags & xbeepacket::FEEDBACK_FLAG_CHICKER_FAULT);
}

void xbee_drive_bot::stamp() {
	assert(shm_frame);
	rwlock_scoped_acquire acq(&ll.shm->lock, &pthread_rwlock_rdlock);
	timespec_now(&shm_frame->timestamp);
}

void xbee_drive_bot::drive_scram() {
	assert(shm_frame);
	rwlock_scoped_acquire acq(&ll.shm->lock, &pthread_rwlock_rdlock);
	shm_frame->run_data.flags &= ~(xbeepacket::RUN_FLAG_DIRECT_DRIVE | xbeepacket::RUN_FLAG_CONTROLLED_DRIVE);
	shm_frame->run_data.drive1_speed = 0;
	shm_frame->run_data.drive2_speed = 0;
	shm_frame->run_data.drive3_speed = 0;
	shm_frame->run_data.drive4_speed = 0;
	timespec_now(&shm_frame->timestamp);
}

void xbee_drive_bot::drive_direct(int m1, int m2, int m3, int m4) {
	assert(shm_frame);
	rwlock_scoped_acquire acq(&ll.shm->lock, &pthread_rwlock_rdlock);
	shm_frame->run_data.flags &= ~xbeepacket::RUN_FLAG_CONTROLLED_DRIVE;
	shm_frame->run_data.flags |= xbeepacket::RUN_FLAG_DIRECT_DRIVE;
	shm_frame->run_data.drive1_speed = smag(m1);
	shm_frame->run_data.drive2_speed = smag(m2);
	shm_frame->run_data.drive3_speed = smag(m3);
	shm_frame->run_data.drive4_speed = smag(m4);
	timespec_now(&shm_frame->timestamp);
}

void xbee_drive_bot::drive_controlled(int m1, int m2, int m3, int m4) {
	assert(shm_frame);
	assert(-1023 <= m1 && m1 <= 1023);
	assert(-1023 <= m2 && m2 <= 1023);
	assert(-1023 <= m3 && m3 <= 1023);
	assert(-1023 <= m4 && m4 <= 1023);
	rwlock_scoped_acquire acq(&ll.shm->lock, &pthread_rwlock_rdlock);
	shm_frame->run_data.flags &= ~xbeepacket::RUN_FLAG_DIRECT_DRIVE;
	shm_frame->run_data.flags |= xbeepacket::RUN_FLAG_CONTROLLED_DRIVE;
	shm_frame->run_data.drive1_speed = m1;
	shm_frame->run_data.drive2_speed = m2;
	shm_frame->run_data.drive3_speed = m3;
	shm_frame->run_data.drive4_speed = m4;
	timespec_now(&shm_frame->timestamp);
}

void xbee_drive_bot::dribble(int power) {
	assert(shm_frame);
	rwlock_scoped_acquire acq(&ll.shm->lock, &pthread_rwlock_rdlock);
	shm_frame->run_data.dribbler_speed = smag(power);
}

void xbee_drive_bot::enable_chicker(bool enable) {
	assert(shm_frame);
	rwlock_scoped_acquire acq(&ll.shm->lock, &pthread_rwlock_rdlock);
	if (enable) {
		shm_frame->run_data.flags |= xbeepacket::RUN_FLAG_CHICKER_ENABLED;
	} else {
		shm_frame->run_data.flags &= ~xbeepacket::RUN_FLAG_CHICKER_ENABLED;
	}
}

void xbee_drive_bot::kick(unsigned int width) {
	assert(shm_frame);
	assert(0 < width && width < 512);
	rwlock_scoped_acquire acq(&ll.shm->lock, &pthread_rwlock_rdlock);
	shm_frame->run_data.flags &= ~xbeepacket::RUN_FLAG_CHIP;
	shm_frame->run_data.chick_power = width;
	Glib::signal_timeout().connect_once(sigc::mem_fun(this, &xbee_drive_bot::clear_chick), 250);
}

void xbee_drive_bot::chip(unsigned int width) {
	assert(shm_frame);
	assert(0 < width && width < 512);
	rwlock_scoped_acquire acq(&ll.shm->lock, &pthread_rwlock_rdlock);
	shm_frame->run_data.flags |= xbeepacket::RUN_FLAG_CHIP;
	shm_frame->run_data.chick_power = width;
	Glib::signal_timeout().connect_once(sigc::mem_fun(this, &xbee_drive_bot::clear_chick), 250);
}

void xbee_drive_bot::on_meta(const void *buffer, std::size_t length) {
	if (length >= sizeof(xbeepacket::META_HDR)) {
		uint8_t metatype = static_cast<const xbeepacket::META_HDR *>(buffer)->metatype;
		
		if (metatype == xbeepacket::CLAIM_FAILED_LOCKED_METATYPE) {
			const xbeepacket::META_CLAIM_FAILED &packet = *static_cast<const xbeepacket::META_CLAIM_FAILED *>(buffer);
			if (length == sizeof(packet)) {
				if (packet.address == address) {
					signal_claim_failed_locked.emit();
				}
			}
		} else if (metatype == xbeepacket::CLAIM_FAILED_RESOURCE_METATYPE) {
			const xbeepacket::META_CLAIM_FAILED &packet = *static_cast<const xbeepacket::META_CLAIM_FAILED *>(buffer);
			if (length == sizeof(packet)) {
				if (packet.address == address) {
					signal_claim_failed_resource.emit();
				}
			}
		} else if (metatype == xbeepacket::ALIVE_METATYPE) {
			const xbeepacket::META_ALIVE &packet = *static_cast<const xbeepacket::META_ALIVE *>(buffer);
			if (length == sizeof(packet)) {
				if (packet.address == address) {
					assert(packet.shm_frame < xbeepacket::MAX_DRIVE_ROBOTS);
					shm_frame = &ll.shm->frames[packet.shm_frame];
					alive_ = true;
					signal_alive.emit();
				}
			}
		} else if (metatype == xbeepacket::DEAD_METATYPE) {
			const xbeepacket::META_DEAD &packet = *static_cast<const xbeepacket::META_DEAD *>(buffer);
			if (length == sizeof(packet)) {
				if (packet.address == address) {
					alive_ = false;
					signal_dead.emit();
				}
			}
		} else if (metatype == xbeepacket::FEEDBACK_METATYPE) {
			const xbeepacket::META_FEEDBACK &packet = *static_cast<const xbeepacket::META_FEEDBACK *>(buffer);
			if (length == sizeof(packet)) {
				if (packet.address == address) {
					{
						rwlock_scoped_acquire acq(&ll.shm->lock, &pthread_rwlock_rdlock);
						feedback_ = shm_frame->feedback_data;
						latency_ = shm_frame->latency;
						inbound_rssi_ = shm_frame->inbound_rssi;
						success_rate_ = __builtin_popcountll(shm_frame->delivery_mask);
					}
					signal_feedback.emit();
				}
			}
		}
	}
}

void xbee_drive_bot::clear_chick() {
	assert(shm_frame);
	rwlock_scoped_acquire acq(&ll.shm->lock, &pthread_rwlock_rdlock);
	shm_frame->run_data.flags &= ~xbeepacket::RUN_FLAG_CHIP;
	shm_frame->run_data.chick_power = 0;
}

