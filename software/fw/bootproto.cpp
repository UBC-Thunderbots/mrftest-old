#include "fw/bootproto.h"
#include "util/xbee.h"
#include "xbee/shared/packettypes.h"
#include <alloca.h>
#include <cassert>

namespace {
	const unsigned int TIMEOUT = 2000;
	const unsigned int MAX_RETRIES = 16;

#warning unportable, replace with objects with proper serialization
	struct __attribute__((packed)) COMMAND_PACKET {
		uint8_t command;
		uint8_t address_high;
		uint8_t address_low;
		uint8_t payload[];
	};

#warning unportable, replace with objects with proper serialization
	struct __attribute__((packed)) RESPONSE_PACKET {
		uint8_t command;
		uint8_t status;
		uint8_t payload[];
	};

	const uint8_t COMMAND_STATUS_OK = 0x00;
}

BootProto::BootProto(XBeeRawBot::Ptr bot) : bot(bot), current_state(STATE_NOT_STARTED) {
}

void BootProto::report_error(const Glib::ustring &error) {
	current_state = STATE_ERROR;
	packet_received_connection.disconnect();
	timeout_connection.disconnect();
	nullary_callback.disconnect();
	response_callback.disconnect();
	signal_error.emit(error);
}

void BootProto::enter_bootloader(const sigc::slot<void> &callback) {
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

void BootProto::enter_bootloader_send() {
	// Check sanity.
	assert(current_state == STATE_ENTERING_BOOTLOADER);

	// Cut old signal connections.
	packet_received_connection.disconnect();

	// Use up a retry.
	if (!retries--) {
		report_error("Cannot enter bootloader mode: No response.");
		return;
	}

	// Send the packet.
	const uint8_t value = 5;
	RemoteATPacket<1>::Ptr pkt(RemoteATPacket<1>::create(bot->address, "D0", &value, true));
	packet_received_connection = pkt->signal_complete().connect(sigc::mem_fun(this, &BootProto::enter_bootloader_complete));
	bot->send(pkt);
}

void BootProto::enter_bootloader_complete(const void *data, std::size_t) {
	// Check sanity.
	assert(current_state == STATE_ENTERING_BOOTLOADER);
	const XBeePacketTypes::REMOTE_AT_RESPONSE &pkt = *static_cast<const XBeePacketTypes::REMOTE_AT_RESPONSE *>(data);
	if (pkt.apiid != XBeePacketTypes::REMOTE_AT_RESPONSE_APIID) {
		return;
	}
	if (XBeeUtil::address_from_bytes(pkt.address64) != bot->address) {
		return;
	}
	if (pkt.command[0] != 'D' || pkt.command[1] != '0') {
		return;
	}

	// Check return status.
	switch (pkt.status) {
		case XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_OK:
			// Command executed successfully.
			// Disconnect signals.
			packet_received_connection.disconnect();

			// Assign the 16-bit address.
			retries = MAX_RETRIES;
			assign_address16_send();
			return;

		case XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_NO_RESPONSE:
			// No response from the remote system.
			// Try resending, if we have retries left.
			enter_bootloader_send();
			return;

		case XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_ERROR:
			// Hard error.
			// Don't bother retrying; just report an error.
			report_error("Cannot enter bootloader mode: Error setting pin state.");
			return;

		case XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_INVALID_COMMAND:
			// Hard error.
			// Don't bother retrying; just report an error.
			report_error("Cannot enter bootloader mode: AT command rejected.");
			return;

		case XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_INVALID_PARAMETER:
			// Hard error.
			// Don't bother retrying; just report an error.
			report_error("Cannot enter bootloader mode: AT command parameter rejected.");
			return;

		default:
			// Hard error.
			// Don't bother retrying; just report an error.
			report_error("Cannot enter bootloader mode: Unknown error.");
			return;
	}
}

void BootProto::assign_address16_send() {
	// Check sanity.
	assert(current_state == STATE_ENTERING_BOOTLOADER);

	// Cut old signal connections.
	packet_received_connection.disconnect();

	// Use up a retry.
	if (!retries--) {
		report_error("Cannot assign 16-bit address: No response.");
		return;
	}

	// Send the packet.
	uint8_t value[2] = { static_cast<uint8_t>(bot->address16() >> 8), static_cast<uint8_t>(bot->address16() & 0xFF) };
	RemoteATPacket<2>::Ptr pkt(RemoteATPacket<2>::create(bot->address, "MY", value, true));
	packet_received_connection = pkt->signal_complete().connect(sigc::mem_fun(this, &BootProto::assign_address16_complete));
	bot->send(pkt);
}

void BootProto::assign_address16_complete(const void *data, std::size_t) {
	// Check sanity.
	assert(current_state == STATE_ENTERING_BOOTLOADER);
	const XBeePacketTypes::REMOTE_AT_RESPONSE &pkt = *static_cast<const XBeePacketTypes::REMOTE_AT_RESPONSE *>(data);
	if (pkt.apiid != XBeePacketTypes::REMOTE_AT_RESPONSE_APIID) {
		return;
	}
	if (XBeeUtil::address_from_bytes(pkt.address64) != bot->address) {
		return;
	}
	if (pkt.command[0] != 'M' || pkt.command[1] != 'Y') {
		return;
	}

	// Check return status.
	switch (pkt.status) {
		case XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_OK:
			// Command executed successfully.
			// Disconnect signals.
			packet_received_connection.disconnect();

			// Give the bootloader time to quiesce before claiming we're ready.
			Glib::signal_timeout().connect(sigc::mem_fun(this, &BootProto::enter_bootloader_quiesce), 500);
			return;

		case XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_NO_RESPONSE:
			// No response from the remote system.
			// Try resending, if we have retries left.
			assign_address16_send();
			return;

		case XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_ERROR:
			// Hard error.
			// Don't bother retrying; just report an error.
			report_error("Cannot enter bootloader mode: Error assigning 16-bit address.");
			return;

		case XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_INVALID_COMMAND:
			// Hard error.
			// Don't bother retrying; just report an error.
			report_error("Cannot enter bootloader mode: AT command rejected.");
			return;

		case XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_INVALID_PARAMETER:
			// Hard error.
			// Don't bother retrying; just report an error.
			report_error("Cannot enter bootloader mode: AT command parameter rejected.");
			return;

		default:
			// Hard error.
			// Don't bother retrying; just report an error.
			report_error("Cannot enter bootloader mode: Unknown error.");
			return;
	}
}

bool BootProto::enter_bootloader_quiesce() {
	// Check sanity.
	assert(current_state == STATE_ENTERING_BOOTLOADER);

	// We're now in the bootloader.
	// Mark state.
	current_state = STATE_READY;

	// Notify the client.
	nullary_callback();

	return false;
}

void BootProto::send_no_response(uint8_t command, uint16_t address, const void *data, std::size_t data_len) {
	// Sanity check.
	assert(current_state == STATE_READY);
	assert(!(command & 0x80));
	assert(data_len <= 97);

	// Cut old signal connections.
	packet_received_connection.disconnect();
	timeout_connection.disconnect();

	// Assemble the packet.
	COMMAND_PACKET *pkt = static_cast<COMMAND_PACKET *>(alloca(sizeof(COMMAND_PACKET) + data_len));
	pkt->command = command;
	pkt->address_high = address / 256;
	pkt->address_low = address % 256;
	if (data_len) {
		std::copy(static_cast<const char *>(data), static_cast<const char *>(data) + data_len, pkt->payload);
	}

	// Send the packet.
	Transmit16Packet::Ptr tx16(Transmit16Packet::create(bot->address16(), false, false, pkt, sizeof(COMMAND_PACKET) + data_len));
	bot->send(tx16);
}

void BootProto::send(uint8_t command, uint16_t address, const void *data, std::size_t data_len, std::size_t response_len, const sigc::slot<void, const void *> &callback) {
	// Sanity check.
	assert(current_state == STATE_READY);
	assert(!(command & 0x80));
	assert(data_len <= 97);

	// Mark new state.
	current_state = STATE_BUSY;

	// Reset retry counter.
	retries = MAX_RETRIES;

	// Assemble the packet.
	COMMAND_PACKET *pkt = static_cast<COMMAND_PACKET *>(alloca(sizeof(COMMAND_PACKET) + data_len));
	pkt->command = command;
	pkt->address_high = address / 256;
	pkt->address_low = address % 256;
	if (data_len) {
		std::copy(static_cast<const char *>(data), static_cast<const char *>(data) + data_len, pkt->payload);
	}
	pending_packet = Transmit16Packet::create(bot->address16(), false, false, pkt, sizeof(COMMAND_PACKET) + data_len);

	// Save the response parameters.
	response_callback = callback;
	pending_response_len = response_len;

	// Send the packet.
	send_send();
}

void BootProto::send_send() {
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
	bot->send(pending_packet);

	// Set up signals.
	packet_received_connection = bot->signal_receive16.connect(sigc::mem_fun(this, &BootProto::send_receive));
	timeout_connection = Glib::signal_timeout().connect(sigc::mem_fun(this, &BootProto::send_timeout), TIMEOUT);
}

bool BootProto::send_timeout() {
	send_send();
	return false;
}

void BootProto::send_receive(uint8_t, const void *data, std::size_t length) {
	// Check sanity.
	assert(current_state == STATE_BUSY);
	if (length < sizeof(RESPONSE_PACKET)) {
		return;
	}
	const RESPONSE_PACKET &pkt = *static_cast<const RESPONSE_PACKET *>(data);

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
			// Hard error.
			// Don't bother retrying; just report an error.
			report_error("Communication error: Unknown error.");
			return;
	}
}

void BootProto::exit_bootloader(const sigc::slot<void> &callback) {
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

void BootProto::exit_bootloader_send() {
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
	const uint8_t value = 4;
	RemoteATPacket<1>::Ptr pkt(RemoteATPacket<1>::create(bot->address, "D0", &value, true));
	pkt->signal_complete().connect(sigc::mem_fun(this, &BootProto::exit_bootloader_complete));
	bot->send(pkt);
}

void BootProto::exit_bootloader_complete(const void *data, std::size_t) {
	// Check sanity.
	assert(current_state == STATE_EXITING_BOOTLOADER);
	const XBeePacketTypes::REMOTE_AT_RESPONSE &pkt = *static_cast<const XBeePacketTypes::REMOTE_AT_RESPONSE *>(data);
	if (pkt.apiid != XBeePacketTypes::REMOTE_AT_RESPONSE_APIID) {
		return;
	}
	if (XBeeUtil::address_from_bytes(pkt.address64) != bot->address) {
		return;
	}
	if (pkt.command[0] != 'D' || pkt.command[1] != '0') {
		return;
	}

	// Check return status.
	switch (pkt.status) {
		case XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_OK:
			// Command executed successfully.
			// Disconnect signals.
			packet_received_connection.disconnect();

			// Mark new state.
			current_state = STATE_DONE;

			// Notify the client.
			nullary_callback();
			return;

		case XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_NO_RESPONSE:
			// No response from the remote system.
			// Try resending, if we have retries left.
			exit_bootloader_send();
			return;

		case XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_ERROR:
			// Hard error.
			// Don't bother retrying; just report an error.
			report_error("Cannot exit bootloader mode: Error setting pin state.");
			return;

		case XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_INVALID_COMMAND:
			// Hard error.
			// Don't bother retrying; just report an error.
			report_error("Cannot exit bootloader mode: AT command rejected.");
			return;

		case XBeePacketTypes::REMOTE_AT_RESPONSE_STATUS_INVALID_PARAMETER:
			// Hard error.
			// Don't bother retrying; just report an error.
			report_error("Cannot exit bootloader mode: AT command parameter rejected.");
			return;

		default:
			// Hard error.
			// Don't bother retrying; just report an error.
			report_error("Cannot exit bootloader mode: Unknown error.");
			return;
	}
}

