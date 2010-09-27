#include "xbee/client/raw.h"
#include "xbee/client/lowlevel.h"
#include "xbee/client/packet.h"
#include <cassert>
#include <cstdlib>

XBeeRawBot::XBeeRawBot(uint64_t address, XBeeLowLevel &ll) : address(address), ll(ll), address16_(0) {
	ll.signal_receive16.connect(sigc::mem_fun(this, &XBeeRawBot::on_receive16));
	ll.signal_meta.connect(sigc::mem_fun(this, &XBeeRawBot::on_meta));
	XBeePacket::Ptr pkt(MetaClaimPacket::create(address, false));
	ll.send(pkt);
}

XBeeRawBot::~XBeeRawBot() {
	ll.send(MetaReleasePacket::create(address));
}

void XBeeRawBot::send(XBeePacket::Ptr p) {
	ll.send(p);
}

uint16_t XBeeRawBot::address16() const {
	return address16_;
}

void XBeeRawBot::on_receive16(uint16_t rx_address, uint8_t rssi, const void *data, std::size_t length) {
	if (rx_address == address16_) {
		signal_receive16.emit(rssi, data, length);
	}
}

void XBeeRawBot::on_meta(const void *buffer, std::size_t length) {
	if (length >= sizeof(XBeePacketTypes::META_HDR)) {
		uint8_t metatype = static_cast<const XBeePacketTypes::META_HDR *>(buffer)->metatype;

		if (metatype == XBeePacketTypes::CLAIM_FAILED_LOCKED_METATYPE) {
			const XBeePacketTypes::META_CLAIM_FAILED &packet = *static_cast<const XBeePacketTypes::META_CLAIM_FAILED *>(buffer);
			if (length == sizeof(packet)) {
				if (packet.address == address) {
					signal_claim_failed.emit();
				}
			}
		} else if (metatype == XBeePacketTypes::CLAIM_FAILED_RESOURCE_METATYPE) {
			const XBeePacketTypes::META_CLAIM_FAILED &packet = *static_cast<const XBeePacketTypes::META_CLAIM_FAILED *>(buffer);
			if (length == sizeof(packet)) {
				if (packet.address == address) {
					signal_claim_failed.emit();
				}
			}
		} else if (metatype == XBeePacketTypes::ALIVE_METATYPE) {
			const XBeePacketTypes::META_ALIVE &packet = *static_cast<const XBeePacketTypes::META_ALIVE *>(buffer);
			if (length == sizeof(packet)) {
				if (packet.address == address) {
					assert(packet.shm_frame == 0xFF);
					address16_ = packet.address16;
					signal_alive.emit();
				}
			}
		}
	}
}

