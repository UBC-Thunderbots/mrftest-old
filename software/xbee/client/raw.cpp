#include "xbee/client/lowlevel.h"
#include "xbee/client/raw.h"
#include <cassert>
#include <cstdlib>

xbee_raw_bot::xbee_raw_bot(uint64_t address, xbee_lowlevel &ll) : address(address), ll(ll) {
	ll.signal_receive64.connect(sigc::mem_fun(this, &xbee_raw_bot::on_receive64));
	ll.signal_meta.connect(sigc::mem_fun(this, &xbee_raw_bot::on_meta));
}

xbee_raw_bot::~xbee_raw_bot() {
	ll.send(meta_release_packet::create(address));
}

void xbee_raw_bot::send(packet::ptr p) {
	ll.send(p);
}

void xbee_raw_bot::on_receive64(uint64_t rx_address, uint8_t rssi, const void *data, std::size_t length) {
	if (rx_address == address) {
		signal_receive64.emit(rssi, data, length);
	}
}

void xbee_raw_bot::on_meta(const void *buffer, std::size_t length) {
	if (length >= sizeof(xbeepacket::META_HDR)) {
		uint8_t metatype = static_cast<const xbeepacket::META_HDR *>(buffer)->metatype;
		
		if (metatype == xbeepacket::CLAIM_FAILED_LOCKED_METATYPE) {
			const xbeepacket::META_CLAIM_FAILED &packet = *static_cast<const xbeepacket::META_CLAIM_FAILED *>(buffer);
			if (length == sizeof(packet)) {
				if (packet.address == address) {
					signal_claim_failed.emit();
				}
			}
		} else if (metatype == xbeepacket::CLAIM_FAILED_RESOURCE_METATYPE) {
			std::abort();
		} else if (metatype == xbeepacket::ALIVE_METATYPE) {
			const xbeepacket::META_ALIVE &packet = *static_cast<const xbeepacket::META_ALIVE *>(buffer);
			if (length == sizeof(packet)) {
				if (packet.address == address) {
					assert(packet.shm_frame == 0xFF);
					signal_alive.emit();
				}
			}
		}
	}
}

