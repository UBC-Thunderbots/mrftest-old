#ifndef XBEE_XBEEMODEM_H
#define XBEE_XBEEMODEM_H

#include "datapool/Noncopyable.h"

#include <cstddef>
#include <stdint.h>

#include <sigc++/sigc++.h>
#include <glibmm.h>

//
// The radio modem. This class only handles packetization and depacketization.
//
class XBeeModem : public sigc::trackable, private virtual Noncopyable {
public:
	//
	// Types of packets (defined by the XBee datasheet).
	//
	enum PACKET {
		PACKET_MODEM_STATUS = 0x8A,
		PACKET_AT_COMMAND = 0x08,
		PACKET_AT_COMMAND_QUEUE = 0x09,
		PACKET_AT_COMMAND_RESPONSE = 0x88,
		PACKET_REMOTE_AT_COMMAND = 0x17,
		PACKET_REMOTE_AT_COMMAND_RESPONSE = 0x97,
		PACKET_TX64 = 0x00,
		PACKET_TX16 = 0x01,
		PACKET_TXSTATUS = 0x89,
		PACKET_RX64 = 0x80,
		PACKET_RX16 = 0x81,
	};

	//
	// Creates a new XBeeModem.
	//
	XBeeModem();

	//
	// Destroys an XBeeModem.
	//
	~XBeeModem();

	//
	// Sends a packet to the modem.
	//
	void send(uint8_t type, const void *payload, std::size_t payloadSize);

	//
	// Connects to the packet-received signal for a particular packet type.
	//
	sigc::connection connect_packet_received(uint8_t type, sigc::slot<void, const void *, std::size_t> slot);

private:
	int fd;
	sigc::signal<void, const void *, std::size_t> signal_packet_received[256];
	uint8_t rxBuf[65535 + 4];
	unsigned int rxBufPtr;

	bool onIO(Glib::IOCondition events);
};

#endif

