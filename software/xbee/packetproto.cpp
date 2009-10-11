#include "xbee/packetproto.h"

xbee_packet_stream::xbee_packet_stream(const Glib::ustring &portname) : bstream(portname), sop_seen(false) {
	bstream.signal_sop_received().connect(sigc::mem_fun(*this, &xbee_packet_stream::on_sop));
	bstream.signal_byte_received().connect(sigc::mem_fun(*this, &xbee_packet_stream::on_byte));
}

void xbee_packet_stream::send(const std::vector<uint8_t> &payload) {
	bstream.send_sop();
	bstream.send(payload.size() / 256);
	bstream.send(payload.size() % 256);
	uint8_t sum = 0;
	for (unsigned int i = 0; i < payload.size(); i++) {
		sum += payload[i];
		bstream.send(payload[i]);
	}
	bstream.send(0xFF - sum);
}

void xbee_packet_stream::on_sop() {
	sop_seen = true;
	length_seen = 0;
	length = 0;
	buffer.clear();
}

void xbee_packet_stream::on_byte(uint8_t ch) {
	if (!sop_seen) {
		/* Do nothing. */
	} else if (length_seen < 2) {
		length *= 256;
		length += ch;
		length_seen++;
	} else if (buffer.size() < length) {
		buffer.push_back(ch);
	} else {
		uint8_t sum = 0;
		for (unsigned int i = 0; i < length; i++)
			sum += buffer[i];
		sum += ch;
		if (sum == 0xFF)
			sig_received.emit(buffer);
		buffer.clear();
		sop_seen = false;
	}
}

