#define DEBUG 0
#include "firmware/bootproto.h"
#include "util/dprint.h"
#include "util/xbee.h"
#include "xbee/packettypes.h"
#include <cassert>

namespace {
	const unsigned int TIMEOUT = 3000;
	const unsigned int MAX_RETRIES = 16;

	struct __attribute__((packed)) COMMAND_PACKET {
		xbeepacket::TRANSMIT_HDR hdr;
		uint8_t command;
		uint8_t address_high;
		uint8_t address_low;
		uint8_t payload[];
	};

	struct __attribute__((packed)) RESPONSE_PACKET {
		xbeepacket::RECEIVE_HDR hdr;
		uint8_t command;
		uint8_t status;
		uint8_t payload[];
	};

	const uint8_t COMMAND_STATUS_OK = 0x00;
}

bootproto::bootproto(xbee &modem, uint64_t bot) : modem(modem), bot(bot), current_state(STATE_NOT_STARTED) {
}

void bootproto::report_error(const Glib::ustring &error) {
	current_state = STATE_ERROR;
	packet_received_connection.disconnect();
	timeout_connection.disconnect();
	nullary_callback.disconnect();
	response_callback.disconnect();
	sig_error.emit(error);
}

void bootproto::enter_bootloader(const sigc::slot<void> &callback) {
	// Sanity check.
	assert(current_state == STATE_NOT_STARTED);

	// Mark new state.
	current_state = STATE_ENTERING_BOOTLOADER;

	// Reset retry counter.
	retries = MAX_RETRIES;

	// Save callback.
	nullary_callback = callback;

	// Send the packet.
	enter_bootloader_send();
}

void bootproto::enter_bootloader_send() {
	// Check sanity.
	assert(current_state == STATE_ENTERING_BOOTLOADER);

	// Cut old signal connections.
	packet_received_connection.disconnect();
	timeout_connection.disconnect();

	// Use up a retry.
	if (!retries--) {
		report_error("Cannot enter bootloader mode: No response.");
		return;
	}

	// Send the packet.
	xbeepacket::REMOTE_AT_REQUEST<1> pkt;
	pkt.apiid = xbeepacket::REMOTE_AT_REQUEST_APIID;
	pkt.frame = 'E';
	xbeeutil::address_to_bytes(bot, pkt.address64);
	pkt.address16[0] = 0xFF;
	pkt.address16[1] = 0xFE;
	pkt.options = xbeepacket::REMOTE_AT_REQUEST_OPTION_APPLY;
	pkt.command[0] = 'D';
	pkt.command[1] = '0';
	pkt.value[0] = 5;
	modem.send(&pkt, sizeof(pkt));

	// Set up signals.
	packet_received_connection = modem.signal_received().connect(sigc::mem_fun(*this, &bootproto::enter_bootloader_receive));
	timeout_connection = Glib::signal_timeout().connect(sigc::mem_fun(*this, &bootproto::enter_bootloader_timeout), TIMEOUT);
}

bool bootproto::enter_bootloader_timeout() {
	enter_bootloader_send();
	return false;
}

void bootproto::enter_bootloader_receive(const void *data, std::size_t length) {
	// Check sanity.
	assert(current_state == STATE_ENTERING_BOOTLOADER);
	if (length < sizeof(xbeepacket::REMOTE_AT_RESPONSE)) {
		DPRINT("packet ignored: too short");
		return;
	}
	const xbeepacket::REMOTE_AT_RESPONSE &pkt = *static_cast<const xbeepacket::REMOTE_AT_RESPONSE *>(data);
	if (pkt.apiid != xbeepacket::REMOTE_AT_RESPONSE_APIID) {
		DPRINT("packet ignored: wrong API id");
		return;
	}
	if (pkt.frame != 'E') {
		DPRINT("packet ignored: wrong frame number");
		return;
	}
	if (xbeeutil::address_from_bytes(pkt.address64) != bot) {
		DPRINT("packet ignored: wrong source address");
		return;
	}
	if (pkt.command[0] != 'D' || pkt.command[1] != '0') {
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

			// Give the bootloader time to quiesce before claiming we're ready.
			Glib::signal_timeout().connect(sigc::mem_fun(*this, &bootproto::enter_bootloader_quiesce), 1000);
			return;

		case xbeepacket::REMOTE_AT_RESPONSE_STATUS_NO_RESPONSE:
			// No response from the remote system. Try resending, if we have retries left.
			enter_bootloader_send();
			return;

		case xbeepacket::REMOTE_AT_RESPONSE_STATUS_ERROR:
			// Hard error. Don't bother retrying; just report an error.
			report_error("Cannot enter bootloader mode: Error setting pin state.");
			return;

		case xbeepacket::REMOTE_AT_RESPONSE_STATUS_INVALID_COMMAND:
			// Hard error. Don't bother retrying; just report an error.
			report_error("Cannot enter bootloader mode: AT command rejected.");
			return;

		case xbeepacket::REMOTE_AT_RESPONSE_STATUS_INVALID_PARAMETER:
			// Hard error. Don't bother retrying; just report an error.
			report_error("Cannot enter bootloader mode: AT command parameter rejected.");
			return;

		default:
			// Hard error. Don't bother retrying; just report an error.
			report_error("Cannot enter bootloader mode: Unknown error.");
			return;
	}
}

bool bootproto::enter_bootloader_quiesce() {
	// Check sanity.
	assert(current_state == STATE_ENTERING_BOOTLOADER);

	// We're now in the bootloader. Mark state.
	current_state = STATE_READY;

	// Notify the client.
	nullary_callback();

	return false;
}

void bootproto::send_no_response(uint8_t command, uint16_t address, const void *data, std::size_t data_len) {
	// Sanity check.
	assert(current_state == STATE_READY);
	assert(!(command & 0x80));
	assert(data_len <= 97);

	// Cut old signal connections.
	packet_received_connection.disconnect();
	timeout_connection.disconnect();

	// Assemble the packet.
	std::vector<uint8_t> packet(sizeof(COMMAND_PACKET) + data_len);
	COMMAND_PACKET *pkt = reinterpret_cast<COMMAND_PACKET *>(&packet[0]);
	pkt->hdr.apiid = xbeepacket::TRANSMIT_APIID;
	pkt->hdr.frame = 0;
	xbeeutil::address_to_bytes(bot, pkt->hdr.address);
	pkt->hdr.options = 0;
	pkt->command = command;
	pkt->address_high = address / 256;
	pkt->address_low = address % 256;
	if (data_len) {
		std::copy(static_cast<const uint8_t *>(data), static_cast<const uint8_t *>(data) + data_len, pkt->payload);
	}

	// Send the packet.
	modem.send(&packet[0], packet.size());
}

void bootproto::send(uint8_t command, uint16_t address, const void *data, std::size_t data_len, std::size_t response_len, const sigc::slot<void, const void *> &callback) {
	// Sanity check.
	assert(current_state == STATE_READY);
	assert(!(command & 0x80));
	assert(data_len <= 97);

	// Mark new state.
	current_state = STATE_BUSY;

	// Reset retry counter.
	retries = MAX_RETRIES;

	// Save data.
	pending_data.resize(sizeof(COMMAND_PACKET) + data_len);
	COMMAND_PACKET *pkt = reinterpret_cast<COMMAND_PACKET *>(&pending_data[0]);
	pkt->hdr.apiid = xbeepacket::TRANSMIT_APIID;
	pkt->hdr.frame = 0;
	xbeeutil::address_to_bytes(bot, pkt->hdr.address);
	pkt->hdr.options = 0;
	pkt->command = command;
	pkt->address_high = address / 256;
	pkt->address_low = address % 256;
	if (data_len) {
		std::copy(static_cast<const uint8_t *>(data), static_cast<const uint8_t *>(data) + data_len, pkt->payload);
	}
	pending_response_len = response_len;
	response_callback = callback;

	// Send the packet.
	send_send();
}

void bootproto::send_send() {
	// Check sanity.
	assert(current_state == STATE_BUSY);

	// Cut old signal connections.
	packet_received_connection.disconnect();
	timeout_connection.disconnect();

	// Use up a retry.
	if (!retries--) {
		report_error("Cannot enter bootloader mode: No response.");
		return;
	}

	// Send the packet.
	modem.send(&pending_data[0], pending_data.size());

	// Set up signals.
	packet_received_connection = modem.signal_received().connect(sigc::mem_fun(*this, &bootproto::send_receive));
	timeout_connection = Glib::signal_timeout().connect(sigc::mem_fun(*this, &bootproto::send_timeout), TIMEOUT);
}

bool bootproto::send_timeout() {
	DPRINT("timeout on send");
	send_send();
	return false;
}

void bootproto::send_receive(const void *data, std::size_t length) {
	// Check sanity.
	assert(current_state == STATE_BUSY);
	if (length < sizeof(RESPONSE_PACKET)) {
		DPRINT("packet ignored: too short");
		return;
	}
	const RESPONSE_PACKET &pkt = *static_cast<const RESPONSE_PACKET *>(data);
	if (pkt.hdr.apiid != xbeepacket::RECEIVE_APIID) {
		DPRINT("packet ignored: wrong API id");
		return;
	}
	if (xbeeutil::address_from_bytes(pkt.hdr.address) != bot) {
		DPRINT("packet ignored: wrong source address");
		return;
	}

	// Check return status.
	switch (pkt.status) {
		case COMMAND_STATUS_OK:
			// Check for expected size.
			if (length - sizeof(RESPONSE_PACKET) != pending_response_len) {
				// Hard error. Report an error.
				report_error("Communication error: Command response was unexpected size.");
				return;
			}

			// Command executed successfully.
			// Disconnect signals.
			packet_received_connection.disconnect();
			timeout_connection.disconnect();

			// Mark new state.
			current_state = STATE_READY;

			// Notify the client.
			response_callback(pkt.payload);
			return;

		default:
			// Hard error. Don't bother retrying; just report an error.
			report_error("Communication error: Unknown error.");
			return;
	}
}

void bootproto::exit_bootloader(const sigc::slot<void> &callback) {
	// Sanity check.
	assert(current_state == STATE_READY);

	// Mark new state.
	current_state = STATE_EXITING_BOOTLOADER;

	// Reset retry counter.
	retries = MAX_RETRIES;

	// Save callback.
	nullary_callback = callback;

	// Send the packet.
	exit_bootloader_send();
}

void bootproto::exit_bootloader_send() {
	// Check sanity.
	assert(current_state == STATE_EXITING_BOOTLOADER);

	// Cut old signal connections.
	packet_received_connection.disconnect();
	timeout_connection.disconnect();

	// Use up a retry.
	if (!retries--) {
		report_error("Cannot exit bootloader mode: No response.");
		return;
	}

	// Send the packet.
	xbeepacket::REMOTE_AT_REQUEST<1> pkt;
	pkt.apiid = xbeepacket::REMOTE_AT_REQUEST_APIID;
	pkt.frame = 'E';
	xbeeutil::address_to_bytes(bot, pkt.address64);
	pkt.address16[0] = 0xFF;
	pkt.address16[1] = 0xFE;
	pkt.options = xbeepacket::REMOTE_AT_REQUEST_OPTION_APPLY;
	pkt.command[0] = 'D';
	pkt.command[1] = '0';
	pkt.value[0] = 4;
	modem.send(&pkt, sizeof(pkt));

	// Set up signals.
	packet_received_connection = modem.signal_received().connect(sigc::mem_fun(*this, &bootproto::exit_bootloader_receive));
	timeout_connection = Glib::signal_timeout().connect(sigc::mem_fun(*this, &bootproto::exit_bootloader_timeout), TIMEOUT);
}

bool bootproto::exit_bootloader_timeout() {
	exit_bootloader_send();
	return false;
}

void bootproto::exit_bootloader_receive(const void *data, std::size_t length) {
	// Check sanity.
	assert(current_state == STATE_EXITING_BOOTLOADER);
	if (length < sizeof(xbeepacket::REMOTE_AT_RESPONSE)) {
		DPRINT("packet ignored: too short");
		return;
	}
	const xbeepacket::REMOTE_AT_RESPONSE &pkt = *static_cast<const xbeepacket::REMOTE_AT_RESPONSE *>(data);
	if (pkt.apiid != xbeepacket::REMOTE_AT_RESPONSE_APIID) {
		DPRINT("packet ignored: wrong API id");
		return;
	}
	if (pkt.frame != 'E') {
		DPRINT("packet ignored: wrong frame number");
		return;
	}
	if (xbeeutil::address_from_bytes(pkt.address64) != bot) {
		DPRINT("packet ignored: wrong source address");
		return;
	}
	if (pkt.command[0] != 'D' || pkt.command[1] != '0') {
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

			// Mark new state.
			current_state = STATE_DONE;

			// Notify the client.
			nullary_callback();
			return;

		case xbeepacket::REMOTE_AT_RESPONSE_STATUS_NO_RESPONSE:
			// No response from the remote system. Try resending, if we have retries left.
			exit_bootloader_send();
			return;

		case xbeepacket::REMOTE_AT_RESPONSE_STATUS_ERROR:
			// Hard error. Don't bother retrying; just report an error.
			report_error("Cannot exit bootloader mode: Error setting pin state.");
			return;

		case xbeepacket::REMOTE_AT_RESPONSE_STATUS_INVALID_COMMAND:
			// Hard error. Don't bother retrying; just report an error.
			report_error("Cannot exit bootloader mode: AT command rejected.");
			return;

		case xbeepacket::REMOTE_AT_RESPONSE_STATUS_INVALID_PARAMETER:
			// Hard error. Don't bother retrying; just report an error.
			report_error("Cannot exit bootloader mode: AT command parameter rejected.");
			return;

		default:
			// Hard error. Don't bother retrying; just report an error.
			report_error("Cannot exit bootloader mode: Unknown error.");
			return;
	}
}

