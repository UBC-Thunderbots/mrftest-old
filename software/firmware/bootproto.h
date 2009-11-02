#ifndef FIRWMARE_BOOTPROTO_H
#define FIRMWARE_BOOTPROTO_H

#include "util/noncopyable.h"
#include "xbee/xbee.h"
#include <vector>
#include <stdint.h>
#include <cstddef>
#include <sigc++/sigc++.h>
#include <glibmm.h>

//
// Handles the lower-level protocol of talking to the bootloader.
//
class bootproto : public virtual noncopyable, public virtual sigc::trackable {
	public:
		//
		// The states the robot can be in.
		//
		enum state {
			// No packets sent yet.
			STATE_NOT_STARTED,
			// Trying to go into bootloader mode.
			STATE_ENTERING_BOOTLOADER,
			// In bootloader mode; waiting for request.
			STATE_READY,
			// In bootloader mode; request processing.
			STATE_BUSY,
			// Exiting bootloader mode.
			STATE_EXITING_BOOTLOADER,
			// Bootloader mode exited.
			STATE_DONE,
			// An error has occurred.
			STATE_ERROR
		};

		//
		// Constructs a new bootproto.
		//
		bootproto(xbee &modem, uint64_t bot);

		//
		// Returns the current state of the bootloader.
		//
		state state() const {
			return current_state;
		}

		//
		// Fired if an error occurs.
		//
		sigc::signal<void, const Glib::ustring &> &signal_error() {
			return sig_error;
		}

		//
		// Causes the robot to begin entering bootloader mode. The provided slot
		// will be invoked when bootloader mode has been entered.
		//
		void enter_bootloader(const sigc::slot<void> &callback);

		//
		// Causes a request to be sent to the robot. The provided slot will be
		// invoked when the robot sends back a response.
		//
		void send(uint8_t command, uint8_t index, const void *data, std::size_t data_len, std::size_t response_len, const sigc::slot<void, const void *> &callback);

		//
		// Causes the robot to begin exiting bootloader mode. The proivded slot
		// will be invoked when bootloader mode has been exited.
		//
		void exit_bootloader(const sigc::slot<void> &callback);

	private:
		xbee &modem;
		uint64_t bot;
		sigc::signal<void, const Glib::ustring &> sig_error;
		enum state current_state;
		sigc::slot<void> nullary_callback;
		sigc::slot<void, const void *> response_callback;
		unsigned int retries;
		std::vector<uint8_t> pending_data;
		std::size_t pending_response_len;

		sigc::connection packet_received_connection;
		sigc::connection timeout_connection;

		void report_error(const Glib::ustring &error);

		void enter_bootloader_send();
		bool enter_bootloader_timeout();
		void enter_bootloader_receive(const void *, std::size_t);
		bool enter_bootloader_quiesce();

		void send_send();
		bool send_timeout();
		void send_receive(const void *, std::size_t);

		void exit_bootloader_send();
		bool exit_bootloader_timeout();
		void exit_bootloader_receive(const void *, std::size_t);
};

#endif

