#include "mrf/dongle.h"
#include "mrf/robot.h"
#include "util/annunciator.h"
#include "util/dprint.h"
#include <algorithm>
#include <cassert>
#include <cstring>
#include <stdexcept>
#include <unistd.h>
#include <unordered_map>
#include <glibmm/convert.h>
#include <glibmm/main.h>
#include <glibmm/ustring.h>

#ifndef CHANNEL
#define CHANNEL 20
#endif

namespace {
	std::unique_ptr<USB::InterruptOutTransfer> create_reliable_message_transfer(USB::DeviceHandle &device, unsigned int robot, uint8_t message_id, const void *data, std::size_t length) {
		assert(robot < 8);
		uint8_t buffer[2 + length];
		buffer[0] = static_cast<uint8_t>(robot);
		buffer[1] = message_id;
		std::memcpy(buffer + 2, data, length);
		std::unique_ptr<USB::InterruptOutTransfer> ptr(new USB::InterruptOutTransfer(device, 2, buffer, sizeof(buffer), 0));
		return ptr;
	}
}

MRFDongle::SendReliableMessageOperation::SendReliableMessageOperation(MRFDongle &dongle, unsigned int robot, const void *data, std::size_t length) : dongle(dongle), message_id(dongle.alloc_message_id()), delivery_status(0xFF), transfer(create_reliable_message_transfer(dongle.device, robot, message_id, data, length)) {
	transfer->signal_done.connect(sigc::mem_fun(this, &SendReliableMessageOperation::out_transfer_done));
	transfer->submit();
	mdr_connection = dongle.signal_message_delivery_report.connect(sigc::mem_fun(this, &SendReliableMessageOperation::message_delivery_report));
}

void MRFDongle::SendReliableMessageOperation::result() const {
	transfer->result();
	switch (delivery_status) {
		case 0x00: break;
		case 0x01: throw NotAssociatedError();
		case 0x02: throw NotAcknowledgedError();
		case 0x03: throw ClearChannelError();
		default: throw std::logic_error("Unknown delivery status");
	}
}

void MRFDongle::SendReliableMessageOperation::out_transfer_done(AsyncOperation &op) {
	if (!op.succeeded()) {
		signal_done.emit(*this);
	}
}

void MRFDongle::SendReliableMessageOperation::message_delivery_report(uint8_t id, uint8_t code) {
	if (id == message_id) {
		mdr_connection.disconnect();
		delivery_status = code;
		dongle.free_message_id(message_id);
		signal_done.emit(*this);
	}
}



MRFDongle::SendReliableMessageOperation::NotAssociatedError::NotAssociatedError() : std::runtime_error("Message sent to robot that is not associated") {
}



MRFDongle::SendReliableMessageOperation::NotAcknowledgedError::NotAcknowledgedError() : std::runtime_error("Message sent to robot not acknowledged") {
}



MRFDongle::SendReliableMessageOperation::ClearChannelError::ClearChannelError() : std::runtime_error("Message sent to robot failed to find clear channel") {
}



#warning should support messages more than 64 bytes
MRFDongle::MRFDongle() : context(), device(context, 0xC057, 0x2579), mdr_transfer(device, 1, 2, true, 0), message_transfer(device, 2, 64, false, 0), drive_dirty(false) {
	for (unsigned int i = 0; i < 8; ++i) {
		robots[i].reset(new MRFRobot(*this, i));
	}
	std::memset(drive_packet, 0, sizeof(drive_packet));
	for (unsigned int i = 0; i < 256; ++i) {
		free_message_ids.push(static_cast<uint8_t>(i));
	}
	if (device.get_configuration() != 2) {
		device.set_configuration(1);
		device.control_no_data(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT, 0x01, CHANNEL, 0, 0);
		device.control_no_data(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT, 0x03, 0, 0, 0);
		device.control_no_data(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT, 0x05, 0x1846, 0, 0);
		static const uint64_t MAC = UINT64_C(0x20cb13bd834ab817);
		device.control_out(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT, 0x07, 0, 0, &MAC, sizeof(MAC), 0);
		device.set_configuration(2);
	}
	device.claim_interface(0);

	mdr_transfer.signal_done.connect(sigc::mem_fun(this, &MRFDongle::handle_mdr));
	mdr_transfer.submit();

	message_transfer.signal_done.connect(sigc::mem_fun(this, &MRFDongle::handle_message));
	message_transfer.submit();
}

MRFDongle::~MRFDongle() {
	drive_submit_connection.disconnect();
	device.release_interface(0);
	device.set_configuration(1);
}

uint8_t MRFDongle::alloc_message_id() {
	if (free_message_ids.empty()) {
		throw std::runtime_error("Out of reliable message IDs");
	}
	uint8_t id = free_message_ids.front();
	free_message_ids.pop();
	return id;
}

void MRFDongle::free_message_id(uint8_t id) {
	free_message_ids.push(id);
}

void MRFDongle::handle_mdr(AsyncOperation<void> &) {
	mdr_transfer.result();
	signal_message_delivery_report.emit(mdr_transfer.data()[0], mdr_transfer.data()[1]);
	mdr_transfer.submit();
}

void MRFDongle::handle_message(AsyncOperation<void> &) {
	message_transfer.result();
	if (message_transfer.size()) {
		unsigned int robot = message_transfer.data()[0];
		robots[robot]->handle_message(message_transfer.data() + 1, message_transfer.size() - 1);
	}
	message_transfer.submit();
}

void MRFDongle::dirty_drive() {
	drive_dirty = true;
	if (!drive_submit_connection.connected()) {
		drive_submit_connection = Glib::signal_idle().connect(sigc::mem_fun(this, &MRFDongle::submit_drive_transfer));
	}
}

bool MRFDongle::submit_drive_transfer() {
	if (!drive_transfer) {
		drive_transfer.reset(new USB::InterruptOutTransfer(device, 1, drive_packet, sizeof(drive_packet), 0));
		drive_transfer->signal_done.connect(sigc::mem_fun(this, &MRFDongle::handle_drive_transfer_done));
		drive_transfer->submit();
		drive_dirty = false;
	}
	return false;
}

void MRFDongle::handle_drive_transfer_done(AsyncOperation<void> &op) {
	op.result();
	drive_transfer.reset();
	if (drive_dirty) {
		submit_drive_transfer();
	}
}

void MRFDongle::send_unreliable(unsigned int robot, const void *data, std::size_t len) {
	uint8_t buffer[len + 1];
	buffer[0] = static_cast<uint8_t>(robot);
	std::memcpy(buffer + 1, data, len);
	std::unique_ptr<USB::InterruptOutTransfer> elt(new USB::InterruptOutTransfer(device, 3, buffer, sizeof(buffer), 0));
	auto i = unreliable_messages.insert(unreliable_messages.end(), std::move(elt));
	(*i)->signal_done.connect(sigc::bind(sigc::mem_fun(this, &MRFDongle::check_unreliable_transfer), i));
	(*i)->submit();
}

void MRFDongle::check_unreliable_transfer(AsyncOperation<void> &, std::list<std::unique_ptr<USB::InterruptOutTransfer>>::iterator iter) {
	(*iter)->result();
	unreliable_messages.erase(iter);
}

