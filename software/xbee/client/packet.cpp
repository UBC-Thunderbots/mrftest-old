#include "xbee/client/packet.h"
#include "util/xbee.h"
#include "xbee/shared/packettypes.h"
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>

void Transmit16Packet::transmit(FileDescriptor::Ptr sock, uint8_t frame) const {
	XBeePacketTypes::TRANSMIT16_HDR hdr;
	hdr.apiid = XBeePacketTypes::TRANSMIT16_APIID;
	hdr.frame = frame;
	hdr.address[0] = dest >> 8;
	hdr.address[1] = dest & 0xFF;
	hdr.options = disable_ack ? XBeePacketTypes::TRANSMIT_OPTION_DISABLE_ACK : 0;

	iovec iov[2];
	iov[0].iov_base = &hdr;
	iov[0].iov_len = sizeof(hdr);
	iov[1].iov_base = const_cast<uint8_t *>(&data[0]);
	iov[1].iov_len = data.size();

	msghdr mh;
	mh.msg_name = 0;
	mh.msg_namelen = 0;
	mh.msg_iov = iov;
	mh.msg_iovlen = 2;
	mh.msg_control = 0;
	mh.msg_controllen = 0;
	mh.msg_flags = 0;

	if (sendmsg(sock->fd(), &mh, MSG_NOSIGNAL) != static_cast<ssize_t>(sizeof(hdr) + data.size())) {
		throw std::runtime_error("Cannot send packet to XBee arbiter!");
	}
}

template<std::size_t value_size>
void ATPacket<value_size>::transmit(FileDescriptor::Ptr sock, uint8_t frame) const {
	XBeePacketTypes::AT_REQUEST<value_size> packet;
	packet.apiid = XBeePacketTypes::AT_REQUEST_APIID;
	packet.frame = frame;
	std::copy(command, command + 2, packet.command);
	std::copy(value, value + value_size, packet.value);

	if (send(sock->fd(), &packet, sizeof(packet), MSG_NOSIGNAL) != static_cast<ssize_t>(sizeof(packet))) {
		throw std::runtime_error("Cannot send packet to XBee arbiter!");
	}
}

// Instantiate the template for the needed values.
template class ATPacket<0>;
template class ATPacket<1>;
template class ATPacket<2>;

template<std::size_t value_size>
void RemoteATPacket<value_size>::transmit(FileDescriptor::Ptr sock, uint8_t frame) const {
	XBeePacketTypes::REMOTE_AT_REQUEST<value_size> packet;
	packet.apiid = XBeePacketTypes::REMOTE_AT_REQUEST_APIID;
	packet.frame = frame;
	XBeeUtil::address_to_bytes(dest, packet.address64);
	packet.address16[0] = 0xFF;
	packet.address16[1] = 0xFE;
	packet.options = apply ? XBeePacketTypes::REMOTE_AT_REQUEST_OPTION_APPLY : 0;
	std::copy(command, command + 2, packet.command);
	std::copy(value, value + value_size, packet.value);

	if (send(sock->fd(), &packet, sizeof(packet), MSG_NOSIGNAL) != static_cast<ssize_t>(sizeof(packet))) {
		throw std::runtime_error("Cannot send packet to XBee arbiter!");
	}
}

// Instantiate the template for the needed values.
template class RemoteATPacket<0>;
template class RemoteATPacket<1>;
template class RemoteATPacket<2>;
template class RemoteATPacket<3>;
template class RemoteATPacket<4>;

void MetaClaimPacket::transmit(FileDescriptor::Ptr sock, uint8_t frame) const {
	assert(!frame);

	XBeePacketTypes::META_CLAIM packet;
	packet.hdr.apiid = XBeePacketTypes::META_APIID;
	packet.hdr.metatype = XBeePacketTypes::CLAIM_METATYPE;
	packet.address = address;
	packet.drive_mode = drive_mode ? 1 : 0;

	if (send(sock->fd(), &packet, sizeof(packet), MSG_NOSIGNAL) != static_cast<ssize_t>(sizeof(packet))) {
		throw std::runtime_error("Cannot send packet to XBee arbiter!");
	}
}

void MetaReleasePacket::transmit(FileDescriptor::Ptr sock, uint8_t frame) const {
	assert(!frame);

	XBeePacketTypes::META_RELEASE packet;
	packet.hdr.apiid = XBeePacketTypes::META_APIID;
	packet.hdr.metatype = XBeePacketTypes::RELEASE_METATYPE;
	packet.address = address;

	if (send(sock->fd(), &packet, sizeof(packet), MSG_NOSIGNAL) != static_cast<ssize_t>(sizeof(packet))) {
		throw std::runtime_error("Cannot send packet to XBee arbiter!");
	}
}

