#ifndef MRF_DONGLE_H
#define MRF_DONGLE_H

/**
 * \file
 *
 * \brief Provides access to an MRF24J40 dongle.
 */

#include "drive/dongle.h"
#include "mrf/robot.h"
#include "util/async_operation.h"
#include "util/libusb.h"
#include "util/noncopyable.h"
#include "util/property.h"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <queue>
#include <utility>
#include <vector>
#include <sigc++/connection.h>
#include <sigc++/signal.h>
#include <sigc++/trackable.h>

/**
 * \brief The dongle.
 */
class MRFDongle final : public Drive::Dongle {
	public:
		/**
		 * \brief An operation to send a reliable message.
		 */
		class SendReliableMessageOperation;

		/**
		 * \brief Emitted when a message is received.
		 *
		 * \param[in] robot the robot who sent the message
		 *
		 * \param[in] data the message
		 *
		 * \param[in] length the length of the message
		 */
		sigc::signal<void, unsigned int, const void *, std::size_t> signal_message_received;

		/**
		 * \brief Constructs a new MRFDongle.
		 */
		explicit MRFDongle();

		/**
		 * \brief Destroys an MRFDongle.
		 */
		~MRFDongle();

		/**
		 * \brief Fetches an individual robot proxy.
		 *
		 * \param[in] i the robot number
		 *
		 * \return the robot proxy object that allows communication with the robot
		 */
		MRFRobot &robot(unsigned int i) override {
			assert(i < 8);
			return *robots[i].get();
		}

		/**
		 * \brief Generates an audible beep on the dongle.
		 *
		 * \param[in] length the length of the beep, in milliseconds (0 to 65,535)
		 */
		void beep(unsigned int length);

		/**
		 * \brief Returns the channel number on which the dongle is communicating.
		 */
		uint8_t channel() const;

		/**
		 * \brief Returns the PAN ID on which the dongle is communicating.
		 */
		uint16_t pan() const;

	private:
		friend class MRFRobot;
		friend class SendReliableMessageOperation;

		USB::Context context;
		USB::DeviceHandle device;
		int radio_interface, configuration_altsetting, normal_altsetting;
		std::unique_ptr<USB::InterfaceClaimer> interface_claimer;
		uint8_t channel_;
		uint16_t pan_;
		std::array<std::unique_ptr<USB::BulkInTransfer>, 32> mdr_transfers;
		std::array<std::unique_ptr<USB::InterruptInTransfer>, 32> message_transfers;
		USB::InterruptInTransfer status_transfer;
		Annunciator::Message rx_fcs_fail_message;
		std::unique_ptr<USB::InterruptOutTransfer> drive_transfer;
		std::list<std::unique_ptr<USB::InterruptOutTransfer>> unreliable_messages;
		std::unique_ptr<MRFRobot> robots[8];
		uint16_t drive_packet[8][4];
		bool drive_dirty;
		sigc::connection drive_submit_connection;
		std::queue<uint8_t> free_message_ids;
		sigc::signal<void, uint8_t, uint8_t> signal_message_delivery_report;
		std::unique_ptr<USB::ControlNoDataTransfer> beep_transfer;
		unsigned int pending_beep_length;
		sigc::connection annunciator_beep_connections[2];

		uint8_t alloc_message_id();
		void free_message_id(uint8_t id);
		void handle_mdrs(AsyncOperation<void> &);
		void handle_message(AsyncOperation<void> &, USB::InterruptInTransfer &transfer);
		void handle_status(AsyncOperation<void> &);
		void dirty_drive();
		bool submit_drive_transfer();
		void handle_drive_transfer_done(AsyncOperation<void> &);
		void send_unreliable(unsigned int robot, const void *data, std::size_t len);
		void check_unreliable_transfer(AsyncOperation<void> &, std::list<std::unique_ptr<USB::InterruptOutTransfer>>::iterator iter);
		void submit_beep();
		void handle_beep_done(AsyncOperation<void> &);
		void handle_annunciator_message_activated();
		void handle_annunciator_message_reactivated(std::size_t index);
};



class MRFDongle::SendReliableMessageOperation final : public AsyncOperation<void>, public sigc::trackable {
	public:
		/**
		 * \brief Thrown if a message cannot be delivered because the recipient robot was not associated.
		 */
		class NotAssociatedError;

		/**
		 * \brief Thrown if a message cannot be delivered because it was not acknowledged by its recipient.
		 */
		class NotAcknowledgedError;

		/**
		 * \brief Thrown if a message cannot be delivered because a clear channel could not be found.
		 */
		class ClearChannelError;

		/**
		 * \brief Queues a message for transmission.
		 *
		 * \param[in] dongle the dongle on which to send the message
		 *
		 * \param[in] robot the robot index to which to send the message
		 *
		 * \param[in] data the data to send (the data is copied into an internal buffer)
		 *
		 * \param[in] len the length of the data, including the header
		 */
		explicit SendReliableMessageOperation(MRFDongle &dongle, unsigned int robot, const void *data, std::size_t len);

		/**
		 * \brief Checks for the success of the operation.
		 *
		 * If the operation failed, this function throws the relevant exception
		 */
		void result() const override;

	private:
		MRFDongle &dongle;
		uint8_t message_id, delivery_status;
		std::unique_ptr<USB::InterruptOutTransfer> transfer;
		sigc::connection mdr_connection;

		void out_transfer_done(AsyncOperation<void> &);
		void message_delivery_report(uint8_t id, uint8_t code);
};



class MRFDongle::SendReliableMessageOperation::NotAssociatedError final : public std::runtime_error {
	public:
		/**
		 * \brief Contructs a NotAssociatedError.
		 */
		explicit NotAssociatedError();
};



class MRFDongle::SendReliableMessageOperation::NotAcknowledgedError final : public std::runtime_error {
	public:
		/**
		 * \brief Contructs a NotAcknowledgedError.
		 */
		explicit NotAcknowledgedError();
};



class MRFDongle::SendReliableMessageOperation::ClearChannelError final : public std::runtime_error {
	public:
		/**
		 * \brief Contructs a ClearChannelError.
		 */
		explicit ClearChannelError();
};



inline uint8_t MRFDongle::channel() const {
	return channel_;
}

inline uint16_t MRFDongle::pan() const {
	return pan_;
}

#endif

