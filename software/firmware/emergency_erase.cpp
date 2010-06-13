#define DEBUG 0
#include "firmware/emergency_erase.h"
#include "util/dprint.h"
#include "util/xbee.h"
#include "xbee/client/packet.h"
#include "xbee/shared/packettypes.h"

emergency_erase::emergency_erase(xbee_raw_bot::ptr bot) : bot(bot) {
}

void emergency_erase::start() {
	status = "Emergency Erasing";
	const uint8_t value = 4;
	remote_at_packet<1>::ptr pkt(remote_at_packet<1>::create(bot->address, "D1", &value, true));
	complete_connection = pkt->signal_complete().connect(sigc::mem_fun(this, &emergency_erase::on_complete));
	bot->send(pkt);
}

void emergency_erase::report_error(const Glib::ustring &error) {
	complete_connection.disconnect();
	signal_error.emit(error);
}

void emergency_erase::on_complete(const void *data, std::size_t) {
	// Check sanity.
	const xbeepacket::REMOTE_AT_RESPONSE &pkt = *static_cast<const xbeepacket::REMOTE_AT_RESPONSE *>(data);
	if (pkt.apiid != xbeepacket::REMOTE_AT_RESPONSE_APIID) {
		DPRINT("packet ignored: wrong API id");
		return;
	}
	if (xbeeutil::address_from_bytes(pkt.address64) != bot->address) {
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
			complete_connection.disconnect();

			// Report completion.
			signal_finished.emit();
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

