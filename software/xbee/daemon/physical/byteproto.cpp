#include "xbee/daemon/physical/byteproto.h"
#include <algorithm>
#include <vector>
#include <cassert>

xbee_byte_stream::xbee_byte_stream() : received_escape(false) {
	port.signal_received().connect(sigc::mem_fun(this, &xbee_byte_stream::bytes_received));
}

void xbee_byte_stream::send_sop() {
	static const uint8_t SOP = 0x7E;
	iovec iov;
	iov.iov_base = const_cast<uint8_t *>(&SOP);
	iov.iov_len = 1;
	port.send(&iov, 1);
}

void xbee_byte_stream::send(const iovec *iov, std::size_t iovlen) {
	static const uint8_t ESCAPE_STRINGS[4][2] = {
		{0x7D, 0x7E ^ 0x20},
		{0x7D, 0x7D ^ 0x20},
		{0x7D, 0x11 ^ 0x20},
		{0x7D, 0x13 ^ 0x20}
	};
	static const uint8_t SPECIAL_CHARS[4] = {0x7E, 0x7D, 0x11, 0x13};
	std::vector<iovec> newiov;

	while (iovlen) {
		const uint8_t *dptr = static_cast<const uint8_t *>(iov->iov_base);
		std::size_t index = std::find_first_of(dptr, dptr + iov->iov_len, SPECIAL_CHARS, SPECIAL_CHARS + sizeof(SPECIAL_CHARS) / sizeof(*SPECIAL_CHARS)) - dptr;
		if (index == iov->iov_len) {
			newiov.push_back(*iov);
			++iov;
			--iovlen;
		} else {
			std::size_t len = iov->iov_len;
			do {
				if (index == 0) {
					std::size_t which = std::find(SPECIAL_CHARS, SPECIAL_CHARS + sizeof(SPECIAL_CHARS) / sizeof(*SPECIAL_CHARS), *dptr) - SPECIAL_CHARS;
					assert(which < sizeof(SPECIAL_CHARS) / sizeof(*SPECIAL_CHARS));
					iovec v;
					v.iov_base = const_cast<uint8_t *>(ESCAPE_STRINGS[which]);
					v.iov_len = 2;
					newiov.push_back(v);
					++dptr;
					--len;
				} else {
					iovec v;
					v.iov_base = const_cast<uint8_t *>(dptr);
					v.iov_len = index;
					newiov.push_back(v);
					dptr += index;
					len -= index;
				}
				index = std::find_first_of(dptr, dptr + len, SPECIAL_CHARS, SPECIAL_CHARS + sizeof(SPECIAL_CHARS) / sizeof(*SPECIAL_CHARS)) - dptr;
			} while (len);
			++iov;
			--iovlen;
		}
	}

	port.send(&newiov[0], newiov.size());
}

void xbee_byte_stream::bytes_received(const void *data, std::size_t len) {
	static const uint8_t SPECIAL[2] = {0x7E, 0x7D};
	const uint8_t *dptr = static_cast<const uint8_t *>(data);

	while (len) {
		std::size_t index = std::find_first_of(dptr, dptr + len, SPECIAL, SPECIAL + sizeof(SPECIAL) / sizeof(*SPECIAL)) - dptr;
		if (received_escape) {
			uint8_t ch = *dptr ^ 0x20;
			sig_bytes_received.emit(&ch, 1);
			received_escape = false;
			++dptr;
			--len;
		} else if (index == 0) {
			if (*dptr == 0x7E) {
				sig_sop_received.emit();
				++dptr;
				--len;
			} else if (*dptr == 0x7D) {
				received_escape = true;
				++dptr;
				--len;
			} else {
				assert(false);
			}
		} else {
			sig_bytes_received.emit(dptr, index);
			dptr += index;
			len -= index;
		}
	}
}

