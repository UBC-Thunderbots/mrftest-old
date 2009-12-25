#define DEBUG 0
#include "firmware/emergency_erase.h"
#include "util/dprint.h"
#include "xbee/packettypes.h"
#include "xbee/util.h"



namespace {
	const unsigned int TIMEOUT = 3000;
}



emergency_erase::emergency_erase(xbee &modem, uint64_t bot) : modem(modem), bot(bot) {
}



void emergency_erase::start() {
	xbeepacket::REMOTE_AT_REQUEST<1> pkt;
	pkt.apiid = xbeepacket::REMOTE_AT_REQUEST_APIID;
	pkt.frame = 'X';
	xbeeutil::address_to_bytes(bot, pkt.address64);
	pkt.address16[0] = 0xFF;
	pkt.address16[1] = 0xFE;
	pkt.options = xbeepacket::REMOTE_AT_REQUEST_OPTION_APPLY;
	pkt.command[0] = 'D';
	pkt.command[1] = '1';
	pkt.value[0] = 4;
	modem.send(&pkt, sizeof(pkt));

	packet_received_connection = modem.signal_received().connect(sigc::mem_fun(*this, &emergency_erase::receive));
	timeout_connection = Glib::signal_timeout().connect(sigc::mem_fun(*this, &emergency_erase::timeout), TIMEOUT);
}



void emergency_erase::report_error(const Glib::ustring &error) {
	packet_received_connection.disconnect();
	timeout_connection.disconnect();
	signal_error().emit(error);
}



bool emergency_erase::timeout() {
	report_error("Cannot signal emergency erase: No response.");
	return false;
}



void emergency_erase::receive(const void *data, std::size_t length) {
	// Check sanity.
	if (length < sizeof(xbeepacket::REMOTE_AT_RESPONSE)) {
		DPRINT("packet ignored: too short");
		return;
	}
	const xbeepacket::REMOTE_AT_RESPONSE &pkt = *static_cast<const xbeepacket::REMOTE_AT_RESPONSE *>(data);
	if (pkt.apiid != xbeepacket::REMOTE_AT_RESPONSE_APIID) {
		DPRINT("packet ignored: wrong API id");
		return;
	}
	if (pkt.frame != 'X') {
		DPRINT("packet ignored: wrong frame number");
		return;
	}
	if (xbeeutil::address_from_bytes(pkt.address64) != bot) {
		DPRINT("packet ignored: wrong source address");
		return;
	}
	if (pkt.command[0] != 'D' || pkt.command[1] != '1') {
		DPRINT("packet ignored: wrong AT command");
		return;
	}

	// Check return status.
	switch (pkt.status) {
		case xbeepacket::REMOTE_AT_RESPONSE_STATUS_OK:
			// Command executed successfully.
			// Disconnect signals.
			packet_received_connection.disconnect();
			timeout_connection.disconnect();

			// Report completion.
			signal_finished().emit();
			return;

		case xbeepacket::REMOTE_AT_RESPONSE_STATUS_NO_RESPONSE:
			// No response from the remote system. Try resending, if we have retries left.
			report_error("Cannot signal emergency erase: No response.");
			return;

		case xbeepacket::REMOTE_AT_RESPONSE_STATUS_ERROR:
			// Hard error. Don't bother retrying; just report an error.
			report_error("Cannot signal emergency erase: Error setting pin state.");
			return;

		case xbeepacket::REMOTE_AT_RESPONSE_STATUS_INVALID_COMMAND:
			// Hard error. Don't bother retrying; just report an error.
			report_error("Cannot signal emergency erase: AT command rejected.");
			return;

		case xbeepacket::REMOTE_AT_RESPONSE_STATUS_INVALID_PARAMETER:
			// Hard error. Don't bother retrying; just report an error.
			report_error("Cannot signal emergency erase: AT command parameter rejected.");
			return;

		default:
			// Hard error. Don't bother retrying; just report an error.
			report_error("Cannot signal emergency erase: Unknown error.");
			return;
	}
}

