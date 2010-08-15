#ifndef FIRMWARE_BOOTPROTO_H
#define FIRMWARE_BOOTPROTO_H

#include "util/noncopyable.h"
#include "xbee/client/raw.h"
#include <stdint.h>
#include <sigc++/sigc++.h>

/**
 * Handles the lower-level protocol of talking to the bootloader.
 */
class BootProto : public NonCopyable, public sigc::trackable {
	public:
		/**
		 * The states the robot can be in.
		 */
		enum State {
			/**
			 * No packets sent yet.
			 */
			STATE_NOT_STARTED,

			/**
			 * Trying to go into bootloader mode.
			 */
			STATE_ENTERING_BOOTLOADER,

			/**
			 * In bootloader mode; waiting for request.
			 */
			STATE_READY,

			/**
			 * In bootloader mode; request processing.
			 */
			STATE_BUSY,

			/**
			 * Exiting bootloader mode.
			 */
			STATE_EXITING_BOOTLOADER,

			/**
			 * Bootloader mode exited.
			 */
			STATE_DONE,

			/**
			 * An error has occurred.
			 */
			STATE_ERROR
		};

		/**
		 * Constructs a new BootProto.
		 *
		 * \param[in] bot the robot to bootload.
		 */
		BootProto(XBeeRawBot::Ptr bot);

		/**
		 * Returns the current state of the bootloader.
		 *
		 * \return the current state.
		 */
		State state() const {
			return current_state;
		}

		/**
		 * Fired if an error occurs.
		 */
		sigc::signal<void, const Glib::ustring &> signal_error;

		/**
		 * Causes the robot to begin entering bootloader mode.
		 *
		 * \param[in] callback a callback to invoke once bootloader mode is
		 * active.
		 */
		void enter_bootloader(const sigc::slot<void> &callback);

		/**
		 * Causes a request to be sent to the robot. The request is such that no
		 * response packet is expected.
		 *
		 * \param[in] command the bootloader command.
		 *
		 * \param[in] address the memory address on which to execute the
		 * command.
		 *
		 * \param[in] data the data associated with the command.
		 *
		 * \param[in] data_len the length, in bytes, of \p data.
		 */
		void send_no_response(uint8_t command, uint16_t address, const void *data, std::size_t data_len);

		/**
		 * Causes a request to be sent to the robot.
		 *
		 * \param[in] command the bootloader command.
		 *
		 * \param[in] address the memory address on which to execute the
		 * command.
		 *
		 * \param[in] data the data associated with the command.
		 *
		 * \param[in] data_len the length, in bytes, of \p data.
		 *
		 * \param[in] response_len the length, in bytes, of the expected
		 * response.
		 *
		 * \param[in] callback a callback to invoke when the response is
		 * received.
		 */
		void send(uint8_t command, uint16_t address, const void *data, std::size_t data_len, std::size_t response_len, const sigc::slot<void, const void *> &callback);

		/**
		 * Causes the robot to begin exiting bootloader mode.
		 *
		 * \param[in] callback a callback to invoke once bootloader mode has
		 * been exited.
		 */
		void exit_bootloader(const sigc::slot<void> &callback);

	private:
		const XBeeRawBot::Ptr bot;
		enum State current_state;
		sigc::slot<void> nullary_callback;
		sigc::slot<void, const void *> response_callback;
		unsigned int retries;
		XBeePacket::Ptr pending_packet;
		std::size_t pending_response_len;

		sigc::connection packet_received_connection;
		sigc::connection timeout_connection;

		void report_error(const Glib::ustring &error);

		void enter_bootloader_send();
		void enter_bootloader_complete(const void *, std::size_t);
		void assign_address16_send();
		void assign_address16_complete(const void *, std::size_t);
		bool enter_bootloader_quiesce();

		void send_send();
		bool send_timeout();
		void send_receive(uint8_t, const void *, std::size_t);

		void exit_bootloader_send();
		void exit_bootloader_complete(const void *, std::size_t);
};

#endif

