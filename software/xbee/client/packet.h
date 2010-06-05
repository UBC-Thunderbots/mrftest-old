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
class packet : public byref {
	public:
		//
		// A pointer to a packet.
		//
		typedef Glib::RefPtr<packet> ptr;

		//
		// Whether or not the radio sends a response to the packet.
		//
		const bool has_response;

		//
		// A signal emitted once the packet is complete. Only available if the
		// packet is expecting a response.
		//
		sigc::signal<void, const void *> &signal_complete() {
			assert(has_response);
			return signal_complete_;
		}

		//
		// Encodes the packet for transmission and sends it over a socket.
		//
		virtual void transmit(const file_descriptor &sock, uint8_t frame) const = 0;

	protected:
		//
		// Constructs a new packet.
		//
		packet(bool has_response) : has_response(has_response) {
		}

	private:
		sigc::signal<void, const void *> signal_complete_;
};

//
// A transmit data packet with a 16-bit address.
//
class transmit16_packet : public packet {
	public:
		//
		// A pointer to a transmit64_packet.
		//
		typedef Glib::RefPtr<transmit16_packet> ptr;

		//
		// Constructs a new transmit16_packet.
		//
		static ptr create(uint16_t dest, bool disable_ack, bool has_response, const void *data, std::size_t length) {
			ptr p(new transmit16_packet(dest, disable_ack, has_response, data, length));
			return p;
		}

		//
		// Encodes the packet for transmission and sends it over a socket.
		//
		void transmit(const file_descriptor &, uint8_t) const;

	private:
		uint16_t dest;
		bool disable_ack;
		std::vector<uint8_t> data;

		transmit16_packet(uint16_t dest, bool disable_ack, bool has_response, const void *data, std::size_t length) : packet(has_response), dest(dest), disable_ack(disable_ack), data(static_cast<const uint8_t *>(data), static_cast<const uint8_t *>(data) + length) {
		}
};

//
// A remote AT command packet.
//
template<std::size_t value_size>
class remote_at_packet : public packet {
	public:
		//
		// A pointer to a remote_at_packet.
		//
		typedef Glib::RefPtr<remote_at_packet<value_size> > ptr;

		//
		// Constructs a new remote_at_packet.
		//
		static ptr create(uint64_t dest, const char *command, const void *value) {
			ptr p(new remote_at_packet<value_size>(dest, command, value));
			return p;
		}

		//
		// Encodes the packet for transmission and sends it over a socket.
		//
		void transmit(const file_descriptor &, uint8_t) const;

	private:
		uint64_t dest;
		char command[2];
		uint8_t value[value_size];

		remote_at_packet(uint64_t dest, const char *command, const void *value) : packet(true), dest(dest) {
			std::copy(command, command + 2, this->command);
			std::copy(static_cast<const uint8_t *>(value), static_cast<const uint8_t *>(value) + value_size, this->value);
		}
};

//
// A meta packet with meta type CLAIM.
//
class meta_claim_packet : public packet {
	public:
		//
		// A pointer to a meta_claim_packet.
		//
		typedef Glib::RefPtr<meta_claim_packet> ptr;

		//
		// Constructs a new meta_claim_packet.
		//
		static ptr create(uint64_t address, bool drive_mode) {
			ptr p(new meta_claim_packet(address, drive_mode));
			return p;
		}

		//
		// Transmits the packet.
		//
		void transmit(const file_descriptor &, uint8_t) const;

	private:
		uint64_t address;
		bool drive_mode;

		meta_claim_packet(uint64_t address, bool drive_mode) : packet(false), address(address), drive_mode(drive_mode) {
		}
};

//
// A meta packet with meta type RELEASE.
//
class meta_release_packet : public packet {
	public:
		//
		// A pointer to a meta_release_packet.
		//
		typedef Glib::RefPtr<meta_release_packet> ptr;

		//
		// Constructs a new meta_release_packet.
		//
		static ptr create(uint64_t address) {
			ptr p(new meta_release_packet(address));
			return p;
		}

		//
		// Transmits the packet.
		//
		void transmit(const file_descriptor &, uint8_t) const;

	private:
		uint64_t address;

		meta_release_packet(uint64_t address) : packet(false), address(address) {
		}
};

#endif

