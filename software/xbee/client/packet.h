#ifndef XBEE_CLIENT_PACKET_H
#define XBEE_CLIENT_PACKET_H

#include "util/byref.h"
#include "util/fd.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <glibmm.h>
#include <stdint.h>

//
// A packet that can be sent to the radio.
//
class XBeePacket : public ByRef {
	public:
		//
		// A pointer to a XBeePacket.
		//
		typedef RefPtr<XBeePacket> Ptr;

		//
		// Whether or not the radio sends a response to the packet.
		//
		const bool has_response;

		//
		// A signal emitted once the packet is complete. Only available if the
		// packet is expecting a response.
		//
		sigc::signal<void, const void *, std::size_t> &signal_complete() {
			assert(has_response);
			return signal_complete_;
		}

		//
		// Encodes the packet for transmission and sends it over a socket.
		//
		virtual void transmit(FileDescriptor::Ptr sock, uint8_t frame) const = 0;

	protected:
		//
		// Constructs a new XBeePacket.
		//
		XBeePacket(bool has_response) : has_response(has_response) {
		}

	private:
		sigc::signal<void, const void *, std::size_t> signal_complete_;
};

//
// A transmit data XBeePacket with a 16-bit address.
//
class Transmit16Packet : public XBeePacket {
	public:
		//
		// A pointer to a transmit64_packet.
		//
		typedef RefPtr<Transmit16Packet> Ptr;

		//
		// Constructs a new Transmit16Packet.
		//
		static Ptr create(uint16_t dest, bool disable_ack, bool has_response, const void *data, std::size_t length) {
			Ptr p(new Transmit16Packet(dest, disable_ack, has_response, data, length));
			return p;
		}

		//
		// Encodes the XBeePacket for transmission and sends it over a socket.
		//
		void transmit(FileDescriptor::Ptr , uint8_t) const;

	private:
		uint16_t dest;
		bool disable_ack;
		std::vector<uint8_t> data;

		Transmit16Packet(uint16_t dest, bool disable_ack, bool has_response, const void *data, std::size_t length) : XBeePacket(has_response), dest(dest), disable_ack(disable_ack), data(static_cast<const uint8_t *>(data), static_cast<const uint8_t *>(data) + length) {
		}
};

/**
 * A local AT command XBeePacket.
 */
template<std::size_t value_size>
class ATPacket : public XBeePacket {
	public:
		/**
		 * A pointer to an ATPacket.
		 */
		typedef RefPtr<ATPacket<value_size> > Ptr;

		/**
		 * Constructs a new ATPacket.
		 *
		 * \param[in] command the two-character command to execute.
		 *
		 * \param[in] value the value to provide.
		 */
		static Ptr create(const char *command, const void *value) {
			Ptr p(new ATPacket<value_size>(command, value));
			return p;
		}

		/**
		 * Encodes the XBeePacket for transmission and sends it over a socket.
		 *
		 * \param[in] sock the socket to send over.
		 *
		 * \param[in] frame the allocated frame number.
		 */
		void transmit(FileDescriptor::Ptr sock, uint8_t frame) const;

	private:
		char command[2];
		uint8_t value[value_size];

		ATPacket(const char *command, const void *value) : XBeePacket(true) {
			std::copy(command, command + 2, this->command);
			std::copy(static_cast<const uint8_t *>(value), static_cast<const uint8_t *>(value) + value_size, this->value);
		}
};

//
// A remote AT command XBeePacket.
//
template<std::size_t value_size>
class RemoteATPacket : public XBeePacket {
	public:
		//
		// A pointer to a RemoteATPacket.
		//
		typedef RefPtr<RemoteATPacket<value_size> > Ptr;

		/**
		 * Constructs a new RemoteATPacket.
		 *
		 * \param[in] dest the address of the robot to send to.
		 *
		 * \param[in] command the two-character command to execute.
		 *
		 * \param[in] value the value to provide.
		 *
		 * \param[in] apply \c true to apply the change immediately, or \c false
		 * to queue the command for later application.
		 */
		static Ptr create(uint64_t dest, const char *command, const void *value, bool apply) {
			Ptr p(new RemoteATPacket<value_size>(dest, command, value, apply));
			return p;
		}

		//
		// Encodes the XBeePacket for transmission and sends it over a socket.
		//
		void transmit(FileDescriptor::Ptr , uint8_t) const;

	private:
		uint64_t dest;
		char command[2];
		uint8_t value[value_size];
		bool apply;

		RemoteATPacket(uint64_t dest, const char *command, const void *value, bool apply) : XBeePacket(true), dest(dest), apply(apply) {
			std::copy(command, command + 2, this->command);
			std::copy(static_cast<const uint8_t *>(value), static_cast<const uint8_t *>(value) + value_size, this->value);
		}
};

//
// A meta XBeePacket with meta type CLAIM.
//
class MetaClaimPacket : public XBeePacket {
	public:
		//
		// A pointer to a MetaClaimPacket.
		//
		typedef RefPtr<MetaClaimPacket> Ptr;

		//
		// Constructs a new MetaClaimPacket.
		//
		static Ptr create(uint64_t address, bool drive_mode) {
			Ptr p(new MetaClaimPacket(address, drive_mode));
			return p;
		}

		//
		// Transmits the XBeePacket.
		//
		void transmit(FileDescriptor::Ptr , uint8_t) const;

	private:
		uint64_t address;
		bool drive_mode;

		MetaClaimPacket(uint64_t address, bool drive_mode) : XBeePacket(false), address(address), drive_mode(drive_mode) {
		}
};

//
// A meta XBeePacket with meta type RELEASE.
//
class MetaReleasePacket : public XBeePacket {
	public:
		//
		// A pointer to a MetaReleasePacket.
		//
		typedef RefPtr<MetaReleasePacket> Ptr;

		//
		// Constructs a new MetaReleasePacket.
		//
		static Ptr create(uint64_t address) {
			Ptr p(new MetaReleasePacket(address));
			return p;
		}

		//
		// Transmits the XBeePacket.
		//
		void transmit(FileDescriptor::Ptr , uint8_t) const;

	private:
		uint64_t address;

		MetaReleasePacket(uint64_t address) : XBeePacket(false), address(address) {
		}
};

#endif

