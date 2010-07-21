#include "fw/emergency_erase.h"
#include "util/xbee.h"
#include "xbee/client/packet.h"
#include "xbee/shared/packettypes.h"

EmergencyErase::EmergencyErase(XBeeRawBot::Ptr bot) : bot(bot) {
}

void EmergencyErase::start() {
	status = "Emergency Erasing";
	const uint8_t value = 4;
	RemoteATPacket<1>::Ptr pkt(RemoteATPacket<1>::create(bot->address, "D1", &value, true));
	complete_connection = pkt->signal_complete().connect(sigc::mem_fun(this, &EmergencyErase::on_complete));
	bot->send(pkt);
}

void EmergencyErase::report_error(const Glib::ustring &error) {
	complete_connection.disconnect();
	signal_error.emit(error);
}

void EmergencyErase::on_complete(const void *data, std::size_t) {
	// Check sanity.
	const XBeePacketTypes::REMOTE_AT_RESPONSE &pkt = *static_cast<const XBeePacketTypes::REMOTE_AT_RESPONSE *>(data);
	if (pkt.apiid != XBeePacketTypes::REMOTE_AT_RESPONSE_APIID) {
		return;
	}
	if (XBeeUtil::address_from_bytes(pkt.address64) != bot->address) {
		return;
	}
	if (pkt.command[0] != 'D' || pkt.command[1] != '1') {
		return;
	}

	// Check return status.
	switch (pkt.status) {
		case XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_OK:
			// Command executed successfully.
			// Disconnect signals.
			complete_connection.disconnect();

			// Report completion.
			signal_finished.emit();
			return;

		case XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_NO_RESPONSE:
			// No response from the remote system. Try resending, if we have retries left.
			report_error("Cannot signal emergency erase: No response.");
			return;

		case XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_ERROR:
			// Hard error. Don't bother retrying; just report an error.
			report_error("Cannot signal emergency erase: Error setting pin state.");
			return;

		case XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_INVALID_COMMAND:
			// Hard error. Don't bother retrying; just report an error.
			report_error("Cannot signal emergency erase: AT command rejected.");
			return;

		case XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_INVALID_PARAMETER:
			// Hard error. Don't bother retrying; just report an error.
			report_error("Cannot signal emergency erase: AT command parameter rejected.");
			return;

		default:
			// Hard error. Don't bother retrying; just report an error.
			report_error("Cannot signal emergency erase: Unknown error.");
			return;
	}
}

