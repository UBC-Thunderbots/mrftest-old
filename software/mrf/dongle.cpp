#include "mrf/dongle.h"
#include "mrf/constants.h"
#include "mrf/robot.h"
#include "util/annunciator.h"
#include "util/dprint.h"
#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <unordered_map>
#include <glibmm/convert.h>
#include <glibmm/main.h>
#include <glibmm/ustring.h>

#define DEFAULT_CHANNEL 20
#define DEFAULT_SYMBOL_RATE 250
#define DEFAULT_PAN 0x1846

namespace {
	const unsigned int ANNUNCIATOR_BEEP_LENGTH = 750;

	std::unique_ptr<USB::InterruptOutTransfer> create_reliable_message_transfer(USB::DeviceHandle &device, unsigned int robot, uint8_t message_id, const void *data, std::size_t length) {
		assert(robot < 8);
		uint8_t buffer[2 + length];
		buffer[0] = static_cast<uint8_t>(robot);
		buffer[1] = message_id;
		std::memcpy(buffer + 2, data, length);
		std::unique_ptr<USB::InterruptOutTransfer> ptr(new USB::InterruptOutTransfer(device, 2, buffer, sizeof(buffer), 64, 0));
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
		case MDR_STATUS_OK: break;
		case MDR_STATUS_NOT_ASSOCIATED: throw NotAssociatedError();
		case MDR_STATUS_NOT_ACKNOWLEDGED: throw NotAcknowledgedError();
		case MDR_STATUS_NO_CLEAR_CHANNEL: throw ClearChannelError();
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



MRFDongle::MRFDongle() : context(), device(context, MRF_DONGLE_VID, MRF_DONGLE_PID, std::getenv("MRF_SERIAL")), mdr_transfer(device, 1, 8, false, 0), message_transfer(device, 2, 103, false, 0), status_transfer(device, 3, 1, true, 0), drive_dirty(false), pending_beep_length(0) {
	for (unsigned int i = 0; i < 8; ++i) {
		robots[i].reset(new MRFRobot(*this, i));
	}
	std::memset(drive_packet, 0, sizeof(drive_packet));
	for (unsigned int i = 0; i < 256; ++i) {
		free_message_ids.push(static_cast<uint8_t>(i));
	}
	device.set_configuration(1);
	{
		USB::InterfaceClaimer temp_interface_claimer(device, 0);
		uint16_t channel = DEFAULT_CHANNEL;
		{
			const char *channel_string = std::getenv("MRF_CHANNEL");
			if (channel_string) {
				int i = std::stoi(channel_string, 0, 0);
				if (i < 0x0B || i > 0x1A) {
					throw std::out_of_range("Channel number must be between 0x0B (11) and 0x1A (26).");
				}
				channel = static_cast<uint16_t>(i);
			}
		}
		int symbol_rate = DEFAULT_SYMBOL_RATE;
		{
			const char *symbol_rate_string = std::getenv("MRF_SYMBOL_RATE");
			if (symbol_rate_string) {
				int i = std::stoi(symbol_rate_string, 0, 0);
				if (i != 250 && i != 625) {
					throw std::out_of_range("Symbol rate must be 250 or 625.");
				}
				symbol_rate = i;
			}
		}
		uint16_t pan = DEFAULT_PAN;
		{
			const char *pan_string = std::getenv("MRF_PAN");
			if (pan_string) {
				int i = std::stoi(pan_string, 0, 0);
				if (i < 0 || i > 0xFFFE) {
					throw std::out_of_range("PAN must be between 0x0000 (0) and 0xFFFE (65,534).");
				}
				pan = static_cast<uint16_t>(i);
			}
		}
		device.control_no_data(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT, CONTROL_REQUEST_SET_CHANNEL, channel, 0, 0);
		device.control_no_data(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT, CONTROL_REQUEST_SET_SYMBOL_RATE, symbol_rate == 625 ? 1 : 0, 0, 0);
		device.control_no_data(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT, CONTROL_REQUEST_SET_PAN_ID, pan, 0, 0);
		static const uint64_t MAC = UINT64_C(0x20cb13bd834ab817);
		device.control_out(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT, CONTROL_REQUEST_SET_MAC_ADDRESS, 0, 0, &MAC, sizeof(MAC), 0);
	}
	config_setter.reset(new USB::ConfigurationSetter(device, 2));
	interface_claimer.reset(new USB::InterfaceClaimer(device, 0));

	mdr_transfer.signal_done.connect(sigc::mem_fun(this, &MRFDongle::handle_mdrs));
	mdr_transfer.submit();

	message_transfer.signal_done.connect(sigc::mem_fun(this, &MRFDongle::handle_message));
	message_transfer.submit();

	status_transfer.signal_done.connect(sigc::mem_fun(this, &MRFDongle::handle_status));
	status_transfer.submit();

	annunciator_beep_connections[0] = Annunciator::signal_message_activated.connect(sigc::mem_fun(this, &MRFDongle::handle_annunciator_message_activated));
	annunciator_beep_connections[1] = Annunciator::signal_message_reactivated.connect(sigc::mem_fun(this, &MRFDongle::handle_annunciator_message_reactivated));
}

MRFDongle::~MRFDongle() {
	annunciator_beep_connections[0].disconnect();
	annunciator_beep_connections[1].disconnect();
	drive_submit_connection.disconnect();
}

void MRFDongle::beep(unsigned int length) {
	pending_beep_length = std::max(length, pending_beep_length);
	if (!beep_transfer && pending_beep_length) {
		beep_transfer.reset(new USB::ControlNoDataTransfer(device, LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE, CONTROL_REQUEST_BEEP, static_cast<uint16_t>(pending_beep_length), 0, 0));
		beep_transfer->signal_done.connect(sigc::mem_fun(this, &MRFDongle::handle_beep_done));
		beep_transfer->submit();
		pending_beep_length = 0;
	}
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

void MRFDongle::handle_mdrs(AsyncOperation<void> &) {
	mdr_transfer.result();
	if ((mdr_transfer.size() % 2) != 0) {
		throw std::runtime_error("MDR transfer has odd size");
	}
	for (unsigned int i = 0; i < mdr_transfer.size(); i += 2) {
		signal_message_delivery_report.emit(mdr_transfer.data()[i], mdr_transfer.data()[i + 1]);
	}
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

void MRFDongle::handle_status(AsyncOperation<void> &) {
	status_transfer.result();
	estop_state = static_cast<EStopState>(status_transfer.data()[0]);
	status_transfer.submit();
}

void MRFDongle::dirty_drive() {
	drive_dirty = true;
	if (!drive_submit_connection.connected()) {
		drive_submit_connection = Glib::signal_idle().connect(sigc::mem_fun(this, &MRFDongle::submit_drive_transfer));
	}
}

bool MRFDongle::submit_drive_transfer() {
	if (!drive_transfer) {
		drive_transfer.reset(new USB::InterruptOutTransfer(device, 1, drive_packet, sizeof(drive_packet), 64, 0));
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
	std::unique_ptr<USB::InterruptOutTransfer> elt(new USB::InterruptOutTransfer(device, 3, buffer, sizeof(buffer), 64, 0));
	auto i = unreliable_messages.insert(unreliable_messages.end(), std::move(elt));
	(*i)->signal_done.connect(sigc::bind(sigc::mem_fun(this, &MRFDongle::check_unreliable_transfer), i));
	(*i)->submit();
}

void MRFDongle::check_unreliable_transfer(AsyncOperation<void> &, std::list<std::unique_ptr<USB::InterruptOutTransfer>>::iterator iter) {
	(*iter)->result();
	unreliable_messages.erase(iter);
}

void MRFDongle::handle_beep_done(AsyncOperation<void> &) {
	beep_transfer->result();
	beep_transfer.reset();
	beep(0);
}

void MRFDongle::handle_annunciator_message_activated() {
	const Annunciator::Message *msg = Annunciator::visible().back();
	if (msg->severity == Annunciator::Message::Severity::HIGH) {
		beep(ANNUNCIATOR_BEEP_LENGTH);
	}
}

void MRFDongle::handle_annunciator_message_reactivated(std::size_t index) {
	const Annunciator::Message *msg = Annunciator::visible()[index];
	if (msg->severity == Annunciator::Message::Severity::HIGH) {
		beep(ANNUNCIATOR_BEEP_LENGTH);
	}
}

