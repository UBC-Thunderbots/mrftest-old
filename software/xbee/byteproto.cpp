#include "xbee/byteproto.h"

xbee_byte_stream::xbee_byte_stream(const Glib::ustring &portname) : port(portname), escape(false) {
	port.signal_received().connect(sigc::mem_fun(*this, &xbee_byte_stream::byte_received));
}

void xbee_byte_stream::send_sop() {
	port.send(0x7E);
}

void xbee_byte_stream::send(uint8_t ch) {
	if (ch == 0x7E || ch == 0x7D || ch == 0x11 || ch == 0x13) {
		port.send(0x7D);
		port.send(ch ^ 0x20);
	} else {
		port.send(ch);
	}
}

void xbee_byte_stream::byte_received(uint8_t ch) {
	if (ch == 0x7E) {
		escape = false;
		sig_sop_received.emit();
	} else if (ch == 0x7D) {
		escape = true;
	} else {
		if (escape) {
			ch ^= 0x20;
			escape = false;
		}
		sig_byte_received.emit(ch);
	}
}

