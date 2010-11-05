// Disable -Wconversion for this file (this file has a lot of such warnings, but it's deprecated and is going away soon).
#pragma GCC diagnostic ignored "-Wconversion"

#include "xbee/daemon/physical/packetproto.h"
#include <algorithm>
#include <cassert>
#include <iterator>
#include <vector>

XBeePacketStream::XBeePacketStream() : sop_seen(false) {
	bstream.signal_sop_received().connect(sigc::mem_fun(this, &XBeePacketStream::on_sop));
	bstream.signal_bytes_received().connect(sigc::mem_fun(this, &XBeePacketStream::on_bytes));
}

void XBeePacketStream::send(const iovec *iov, std::size_t iovcnt) {
	uint8_t checksum = 0xFF;
	std::size_t total_len = 0;
	for (std::size_t i = 0; i < iovcnt; ++i) {
		total_len += iov[i].iov_len;
		for (std::size_t j = 0; j < iov[i].iov_len; ++j) {
			checksum -= static_cast<const uint8_t *>(iov[i].iov_base)[j];
		}
	}

	assert(total_len < 65536);

	uint8_t header[2] = { static_cast<uint8_t>(total_len / 256), static_cast<uint8_t>(total_len % 256) };
	iovec header_iov;
	header_iov.iov_base = header;
	header_iov.iov_len = 2;

	iovec footer_iov;
	footer_iov.iov_base = &checksum;
	footer_iov.iov_len = 1;

	std::vector<iovec> newiov(iovcnt + 2);
	newiov[0] = header_iov;
	std::copy(iov, iov + iovcnt, newiov.begin() + 1);
	newiov[iovcnt + 1] = footer_iov;

	bstream.send_sop();
	bstream.send(&newiov[0], newiov.size());
}

void XBeePacketStream::on_sop() {
	sop_seen = true;
	length_seen = 0;
	length = 0;
	buffer.clear();
}

void XBeePacketStream::on_bytes(const void *bytes, std::size_t len) {
	if (!sop_seen) {
		return;
	}

	const uint8_t *dptr = static_cast<const uint8_t *>(bytes);
	while (len) {
		if (length_seen < 2) {
			length *= 256;
			length += *dptr;
			++length_seen;
			++dptr;
			--len;
		} else if (buffer.size() < length) {
			std::size_t tocopy = std::min(len, length - buffer.size());
			std::size_t pos = buffer.size();
			buffer.resize(pos + tocopy);
			std::copy(dptr, dptr + tocopy, buffer.begin() + pos);
			dptr += tocopy;
			len -= tocopy;
		} else {
			uint8_t sum = 0;
			for (unsigned int i = 0; i < length; ++i) {
				sum += buffer[i];
			}
			sum += *dptr;
			if (sum == 0xFF) {
				signal_received.emit(buffer);
			}
			sop_seen = false;
			return;
		}
	}
}

