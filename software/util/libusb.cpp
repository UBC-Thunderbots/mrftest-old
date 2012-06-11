#include "util/libusb.h"
#include "util/dprint.h"
#include "util/exception.h"
#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <poll.h>
#include <glibmm/convert.h>
#include <glibmm/main.h>
#include <glibmm/ustring.h>

#warning needs Doxygen

namespace {
	long check_fn(const char *call, long err, unsigned int endpoint) {
		if (err >= 0) {
			return err;
		}

		const char *msg;
		switch (err) {
			case LIBUSB_ERROR_IO:
				msg = "Input/output error";
				break;

			case LIBUSB_ERROR_INVALID_PARAM:
				msg = "Invalid parameter";
				break;

			case LIBUSB_ERROR_ACCESS:
				msg = "Access denied (insufficient permissions)";
				break;

			case LIBUSB_ERROR_NO_DEVICE:
				msg = "No such device (it may have been disconnected)";
				break;

			case LIBUSB_ERROR_NOT_FOUND:
				msg = "Entity not found";
				break;

			case LIBUSB_ERROR_BUSY:
				msg = "Resource busy";
				break;

			case LIBUSB_ERROR_TIMEOUT:
				throw USB::TransferTimeoutError(endpoint);
				break;

			case LIBUSB_ERROR_OVERFLOW:
				msg = "Overflow";
				break;

			case LIBUSB_ERROR_PIPE:
				throw USB::TransferStallError(endpoint);
				break;

			case LIBUSB_ERROR_INTERRUPTED:
				msg = "System call interrupted (perhaps due to signal)";
				break;

			case LIBUSB_ERROR_NO_MEM:
				throw std::bad_alloc();
				break;

			case LIBUSB_ERROR_NOT_SUPPORTED:
				msg = "Operation not supported or unimplemented on this platform";
				break;

			case LIBUSB_ERROR_OTHER:
				msg = "Other error";
				break;

			default:
				throw ErrorMessageError();
		}
		throw USB::Error(Glib::locale_from_utf8(Glib::ustring::compose("%1: %2", call, msg)));
	}

	Glib::ustring make_transfer_error_message(unsigned int endpoint, const Glib::ustring &msg) {
		return Glib::ustring::compose("%1 on %2 endpoint %3", msg, (endpoint & 0x80) ? "IN" : "OUT", endpoint & 0x7F);
	}

	bool matches_vid_pid_serial(const USB::Device &device, unsigned int vendor_id, unsigned int product_id, const char *serial_number) {
		if (device.vendor_id() != vendor_id || device.product_id() != product_id) {
			return false;
		}

		if (!serial_number) {
			return true;
		}

		return device.serial_number() == serial_number;
	}
}

USB::Error::Error(const std::string &msg) : std::runtime_error(msg) {
}

USB::TransferError::TransferError(unsigned int endpoint, const std::string &msg) : Error(make_transfer_error_message(endpoint, msg)) {
}

USB::TransferTimeoutError::TransferTimeoutError(unsigned int endpoint) : TransferError(endpoint, "Transfer timed out") {
}

USB::TransferStallError::TransferStallError(unsigned int endpoint) : TransferError(endpoint, "Transfer stalled") {
}

USB::TransferCancelledError::TransferCancelledError(unsigned int endpoint) : TransferError(endpoint, "Transfer cancelled") {
}



USB::Context::Context() {
	check_fn("libusb_init", libusb_init(&context), 0);
	const libusb_pollfd **pfds = libusb_get_pollfds(context);
	if (!pfds) {
		check_fn("libusb_get_pollfds", LIBUSB_ERROR_OTHER, 0);
	}
	for (const libusb_pollfd **i = pfds; *i; ++i) {
		add_pollfd((*i)->fd, (*i)->events);
	}
	std::free(pfds);
	libusb_set_pollfd_notifiers(context, &Context::pollfd_add_trampoline, &Context::pollfd_remove_trampoline, this);
}

USB::Context::~Context() {
	libusb_exit(context);
	context = 0;
	for (auto i = fd_connections.begin(), iend = fd_connections.end(); i != iend; ++i) {
		i->second.disconnect();
	}
}

void USB::Context::pollfd_add_trampoline(int fd, short events, void *user_data) {
	static_cast<Context *>(user_data)->add_pollfd(fd, events);
}

void USB::Context::pollfd_remove_trampoline(int fd, void *user_data) {
	static_cast<Context *>(user_data)->remove_pollfd(fd);
}

void USB::Context::add_pollfd(int fd, short events) {
	auto old = fd_connections.find(fd);
	if (old != fd_connections.end()) {
		old->second.disconnect();
	}
	Glib::IOCondition cond = static_cast<Glib::IOCondition>(0);
	if (events & POLLIN) {
		cond |= Glib::IO_IN;
	}
	if (events & POLLOUT) {
		cond |= Glib::IO_OUT;
	}
	fd_connections[fd] = Glib::signal_io().connect(sigc::bind_return(sigc::hide(sigc::mem_fun(this, &Context::handle_usb_fds)), true), fd, cond);
}

void USB::Context::remove_pollfd(int fd) {
	auto i = fd_connections.find(fd);
	if (i != fd_connections.end()) {
		i->second.disconnect();
		fd_connections.erase(i);
	}
}

void USB::Context::handle_usb_fds() {
	timeval tv = { 0, 0 };
	check_fn("libusb_handle_events_timeout", libusb_handle_events_timeout(context, &tv), 0);
}



USB::Device::Device(const Device &copyref) : device(libusb_ref_device(copyref.device)) {
	check_fn("libusb_get_device_descriptor", libusb_get_device_descriptor(device, &device_descriptor), 0);
}

USB::Device::~Device() {
	libusb_unref_device(device);
}

USB::Device &USB::Device::operator=(const Device &assgref) {
	if (assgref.device != device) {
		libusb_unref_device(device);
		device = libusb_ref_device(assgref.device);
	}
	return *this;
}

USB::Device::Device(libusb_device *device) : device(libusb_ref_device(device)) {
	check_fn("libusb_get_device_descriptor", libusb_get_device_descriptor(device, &device_descriptor), 0);
}

std::string USB::Device::serial_number() const {
	if (!device_descriptor.iSerialNumber) {
		return "";
	}

	std::string value;
	{
		DeviceHandle devh(*this);
		value = devh.get_string_descriptor(device_descriptor.iSerialNumber);
	}
	return value;
}



USB::DeviceList::DeviceList(Context &context) {
	ssize_t ssz;
	check_fn("libusb_get_device_list", ssz = libusb_get_device_list(context.context, &devices), 0);
	size_ = ssz;
}

USB::DeviceList::~DeviceList() {
	libusb_free_device_list(devices, 1);
}

USB::Device USB::DeviceList::operator[](const std::size_t i) const {
	assert(i < size());
	return Device(devices[i]);
}



USB::DeviceHandle::DeviceHandle(const Device &device) : context(device.context), submitted_transfer_count(0) {
	check_fn("libusb_open", libusb_open(device.device, &handle), 0);
}

USB::DeviceHandle::DeviceHandle(Context &context, unsigned int vendor_id, unsigned int product_id, const char *serial_number) : context(context.context), submitted_transfer_count(0) {
	DeviceList lst(context);
	for (std::size_t i = 0; i < lst.size(); ++i) {
		const Device &device = lst[i];
		if (matches_vid_pid_serial(device, vendor_id, product_id, serial_number)) {
			for (std::size_t j = i + 1; j < lst.size(); ++j) {
				const Device &device = lst[j];
				if (matches_vid_pid_serial(device, vendor_id, product_id, serial_number)) {
					throw std::runtime_error("Multiple matching USB devices attached");
				}
			}

			check_fn("libusb_open", libusb_open(device.device, &handle), 0);
			return;
		}
	}

	throw std::runtime_error("No matching USB devices attached");
}

USB::DeviceHandle::~DeviceHandle() {
	while (submitted_transfer_count) {
		check_fn("libusb_handle_events", libusb_handle_events(context), 0);
	}
	libusb_close(handle);
}

#include <iostream>
std::string USB::DeviceHandle::get_string_descriptor(uint8_t index) const {
	std::vector<unsigned char> buf(8);
	int rc;
	do {
		buf.resize(buf.size() * 2);
		rc = libusb_get_string_descriptor_ascii(handle, index, &buf[0], static_cast<int>(buf.size()));
	} while (rc >= static_cast<int>(buf.size()) - 1);
	check_fn("libusb_get_string_descriptor_ascii", rc, 0);
	return std::string(buf.begin(), buf.begin() + rc);
}

int USB::DeviceHandle::get_configuration() const {
	int conf;
	check_fn("libusb_get_configuration", libusb_get_configuration(handle, &conf), 0);
	return conf;
}

void USB::DeviceHandle::set_configuration(int config) {
	check_fn("libusb_set_configuration", libusb_set_configuration(handle, config), 0);
}

void USB::DeviceHandle::claim_interface(int interface) {
	check_fn("libusb_claim_interface", libusb_claim_interface(handle, interface), 0);
}

void USB::DeviceHandle::release_interface(int interface) {
	check_fn("libusb_release_interface", libusb_release_interface(handle, interface), 0);
}

void USB::DeviceHandle::control_no_data(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, unsigned int timeout) {
	assert((request_type & LIBUSB_ENDPOINT_DIR_MASK) == 0);
	check_fn("libusb_control_transfer", libusb_control_transfer(handle, request_type | LIBUSB_ENDPOINT_OUT, request, value, index, 0, 0, timeout), 0);
}

std::size_t USB::DeviceHandle::control_in(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, void *buffer, std::size_t len, unsigned int timeout) {
	assert((request_type & LIBUSB_ENDPOINT_DIR_MASK) == 0);
	assert(len < 65536);
	return check_fn("libusb_control_transfer", libusb_control_transfer(handle, request_type | LIBUSB_ENDPOINT_IN, request, value, index, static_cast<unsigned char *>(buffer), static_cast<uint16_t>(len), timeout), 0);
}

void USB::DeviceHandle::control_out(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, const void *buffer, std::size_t len, unsigned int timeout) {
	assert((request_type & LIBUSB_ENDPOINT_DIR_MASK) == 0);
	assert(len < 65536);
	check_fn("libusb_control_transfer", libusb_control_transfer(handle, request_type | LIBUSB_ENDPOINT_OUT, request, value, index, static_cast<unsigned char *>(const_cast<void *>(buffer)), static_cast<uint16_t>(len), timeout), 0);
}

std::size_t USB::DeviceHandle::interrupt_in(unsigned char endpoint, void *data, std::size_t length, unsigned int timeout) {
	assert((endpoint & LIBUSB_ENDPOINT_ADDRESS_MASK) == endpoint);
	assert(length < static_cast<std::size_t>(std::numeric_limits<int>::max()));
	int transferred = -1;
	check_fn("libusb_interrupt_transfer", libusb_interrupt_transfer(handle, endpoint | LIBUSB_ENDPOINT_IN, static_cast<unsigned char *>(data), static_cast<int>(length), &transferred, timeout), endpoint | LIBUSB_ENDPOINT_IN);
	assert(transferred >= 0);
	return transferred;
}

void USB::DeviceHandle::interrupt_out(unsigned char endpoint, const void *data, std::size_t length, unsigned int timeout) {
	assert((endpoint & LIBUSB_ENDPOINT_ADDRESS_MASK) == endpoint);
	assert(length < static_cast<std::size_t>(std::numeric_limits<int>::max()));
	int transferred;
	check_fn("libusb_interrupt_transfer", libusb_interrupt_transfer(handle, endpoint | LIBUSB_ENDPOINT_OUT, const_cast<unsigned char *>(static_cast<const unsigned char *>(data)), static_cast<int>(length), &transferred, timeout), endpoint | LIBUSB_ENDPOINT_OUT);
	if (transferred != static_cast<int>(length)) {
		throw TransferError(1, "Device accepted wrong amount of data");
	}
}

std::size_t USB::DeviceHandle::bulk_in(unsigned char endpoint, void *data, std::size_t length, unsigned int timeout) {
	assert((endpoint & LIBUSB_ENDPOINT_ADDRESS_MASK) == endpoint);
	assert(length < static_cast<std::size_t>(std::numeric_limits<int>::max()));
	int transferred = -1;
	check_fn("libusb_bulk_transfer", libusb_bulk_transfer(handle, endpoint | LIBUSB_ENDPOINT_IN, static_cast<unsigned char *>(data), static_cast<uint16_t>(length), &transferred, timeout), endpoint | LIBUSB_ENDPOINT_IN);
	assert(transferred >= 0);
	return transferred;
}



class TransferMetadata : public NonCopyable {
	public:
		static TransferMetadata *get(libusb_transfer *transfer) {
			return static_cast<TransferMetadata *>(transfer->user_data);
		}

		TransferMetadata(USB::Transfer &transfer, USB::DeviceHandle &device) : transfer_(&transfer), device_(device) {
		}

		USB::Transfer *transfer() const {
			return transfer_;
		}

		USB::DeviceHandle &device() const {
			return device_;
		}

		void disown() {
			transfer_ = 0;
		}

	private:
		USB::Transfer *transfer_;
		USB::DeviceHandle &device_;
};

USB::Transfer::~Transfer() {
	if (submitted_) {
		// The transfer is submitted.
		// Initiate transfer cancellation.
		// Instead of waiting for cancellation to complete, "disown" the transfer object.
		// It will be freed by the trampoline.
		libusb_cancel_transfer(transfer);
		TransferMetadata::get(transfer)->disown();
	} else {
		// The transfer is not submitted and therefore can safely be freed.
		delete TransferMetadata::get(transfer);
		delete [] transfer->buffer;
		libusb_free_transfer(transfer);
	}
}

void USB::Transfer::result() const {
	assert(done_);
	switch (transfer->status) {
		case LIBUSB_TRANSFER_COMPLETED:
			return;

		case LIBUSB_TRANSFER_ERROR:
			throw TransferError(transfer->endpoint, "Transfer error");

		case LIBUSB_TRANSFER_TIMED_OUT:
			throw TransferTimeoutError(transfer->endpoint);

		case LIBUSB_TRANSFER_CANCELLED:
			throw TransferCancelledError(transfer->endpoint);

		case LIBUSB_TRANSFER_STALL:
			throw TransferStallError(transfer->endpoint);

		case LIBUSB_TRANSFER_NO_DEVICE:
			throw TransferError(transfer->endpoint, "Device was disconnected");

		case LIBUSB_TRANSFER_OVERFLOW:
			throw TransferError(transfer->endpoint, "Device sent more data than requested");

		default:
			throw ErrorMessageError();
	}
}

void USB::Transfer::submit() {
	assert(!submitted_);
	check_fn("libusb_submit_transfer", libusb_submit_transfer(transfer), transfer->endpoint);
	submitted_ = true;
	done_ = false;
	++device.submitted_transfer_count;
}

void USB::Transfer::handle_completed_transfer_trampoline(libusb_transfer *transfer) {
	TransferMetadata *md = TransferMetadata::get(transfer);
	--md->device().submitted_transfer_count;
	if (md->transfer()) {
		md->transfer()->handle_completed_transfer();
	} else {
		// This happens if the Transfer object has been destroyed but the transfer was submitted at the time.
		// The disowned libusb_transfer needs to be allowed to finish cancelling before being freed.
		delete md;
		delete [] transfer->buffer;
		libusb_free_transfer(transfer);
	}
}

USB::Transfer::Transfer(DeviceHandle &dev) : device(dev), transfer(libusb_alloc_transfer(0)), submitted_(false), done_(false) {
	if (!transfer) {
		throw std::bad_alloc();
	}
	try {
		transfer->user_data = new TransferMetadata(*this, dev);
	} catch (...) {
		libusb_free_transfer(transfer);
		throw;
	}
	transfer->flags = 0;
}

void USB::Transfer::handle_completed_transfer() {
	done_ = true;
	submitted_ = false;
	signal_done.emit(*this);
}



USB::InterruptInTransfer::InterruptInTransfer(DeviceHandle &dev, unsigned char endpoint, std::size_t len, bool exact_len, unsigned int timeout) : Transfer(dev) {
	assert((endpoint & LIBUSB_ENDPOINT_ADDRESS_MASK) == endpoint);
	libusb_fill_interrupt_transfer(transfer, dev.handle, endpoint | LIBUSB_ENDPOINT_IN, new unsigned char[len], static_cast<int>(len), &Transfer::handle_completed_transfer_trampoline, transfer->user_data, timeout);
	if (exact_len) {
		transfer->flags |= LIBUSB_TRANSFER_SHORT_NOT_OK;
	}
}



USB::InterruptOutTransfer::InterruptOutTransfer(DeviceHandle &dev, unsigned char endpoint, const void *data, std::size_t len, unsigned int timeout) : Transfer(dev) {
	assert((endpoint & LIBUSB_ENDPOINT_ADDRESS_MASK) == endpoint);
	libusb_fill_interrupt_transfer(transfer, dev.handle, endpoint | LIBUSB_ENDPOINT_OUT, new unsigned char[len], static_cast<int>(len), &Transfer::handle_completed_transfer_trampoline, transfer->user_data, timeout);
	std::memcpy(transfer->buffer, data, len);
}

