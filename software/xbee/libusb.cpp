#include "xbee/libusb.h"
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
	long check_fn(const char *call, long err) {
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
				msg = "Operation timed out";
				break;

			case LIBUSB_ERROR_OVERFLOW:
				msg = "Overflow";
				break;

			case LIBUSB_ERROR_PIPE:
				msg = "Pipe error";
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
		throw LibUSBError(Glib::locale_from_utf8(Glib::ustring::compose("%1: %2", call, msg)));
	}

	Glib::ustring make_transfer_error_message(unsigned int endpoint, const Glib::ustring &msg) {
		return Glib::ustring::compose("%1 on %2 endpoint %3", msg, (endpoint & 0x80) ? "IN" : "OUT", endpoint & 0x7F);
	}
}

LibUSBError::LibUSBError(const std::string &msg) : std::runtime_error(msg) {
}

LibUSBTransferError::LibUSBTransferError(unsigned int endpoint, const std::string &msg) : LibUSBError(make_transfer_error_message(endpoint, msg)) {
}

LibUSBTransferTimeoutError::LibUSBTransferTimeoutError(unsigned int endpoint) : LibUSBTransferError(endpoint, "Transfer timed out") {
}

LibUSBTransferStallError::LibUSBTransferStallError(unsigned int endpoint) : LibUSBTransferError(endpoint, "Transfer stalled") {
}

LibUSBTransferCancelledError::LibUSBTransferCancelledError(unsigned int endpoint) : LibUSBTransferError(endpoint, "Transfer cancelled") {
}



LibUSBContext::LibUSBContext() {
	check_fn("libusb_init", libusb_init(&context));
	const libusb_pollfd **pfds = libusb_get_pollfds(context);
	if (!pfds) {
		check_fn("libusb_get_pollfds", LIBUSB_ERROR_OTHER);
	}
	for (const libusb_pollfd **i = pfds; *i; ++i) {
		add_pollfd((*i)->fd, (*i)->events);
	}
	std::free(pfds);
	libusb_set_pollfd_notifiers(context, &LibUSBContext::pollfd_add_trampoline, &LibUSBContext::pollfd_remove_trampoline, this);
}

LibUSBContext::~LibUSBContext() {
	libusb_exit(context);
	context = 0;
	for (auto i = fd_connections.begin(), iend = fd_connections.end(); i != iend; ++i) {
		i->second.disconnect();
	}
}

void LibUSBContext::pollfd_add_trampoline(int fd, short events, void *user_data) {
	static_cast<LibUSBContext *>(user_data)->add_pollfd(fd, events);
}

void LibUSBContext::pollfd_remove_trampoline(int fd, void *user_data) {
	static_cast<LibUSBContext *>(user_data)->remove_pollfd(fd);
}

void LibUSBContext::add_pollfd(int fd, short events) {
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
	fd_connections[fd] = Glib::signal_io().connect(sigc::bind_return(sigc::hide(sigc::mem_fun(this, &LibUSBContext::handle_usb_fds)), true), fd, cond);
}

void LibUSBContext::remove_pollfd(int fd) {
	auto i = fd_connections.find(fd);
	if (i != fd_connections.end()) {
		i->second.disconnect();
		fd_connections.erase(i);
	}
}

void LibUSBContext::handle_usb_fds() {
	timeval tv = { 0, 0 };
	check_fn("libusb_handle_events_timeout", libusb_handle_events_timeout(context, &tv));
}



LibUSBDevice::LibUSBDevice(const LibUSBDevice &copyref) : device(libusb_ref_device(copyref.device)) {
	check_fn("libusb_get_device_descriptor", libusb_get_device_descriptor(device, &device_descriptor));
}

LibUSBDevice::~LibUSBDevice() {
	libusb_unref_device(device);
}

LibUSBDevice &LibUSBDevice::operator=(const LibUSBDevice &assgref) {
	if (assgref.device != device) {
		libusb_unref_device(device);
		device = libusb_ref_device(assgref.device);
	}
	return *this;
}

LibUSBDevice::LibUSBDevice(libusb_device *device) : device(libusb_ref_device(device)) {
	check_fn("libusb_get_device_descriptor", libusb_get_device_descriptor(device, &device_descriptor));
}



LibUSBDeviceList::LibUSBDeviceList(LibUSBContext &context) {
	ssize_t ssz;
	check_fn("libusb_get_device_list", ssz = libusb_get_device_list(context.context, &devices));
	size_ = ssz;
}

LibUSBDeviceList::~LibUSBDeviceList() {
	libusb_free_device_list(devices, 1);
}

LibUSBDevice LibUSBDeviceList::operator[](const std::size_t i) const {
	assert(i < size());
	return LibUSBDevice(devices[i]);
}



LibUSBDeviceHandle::LibUSBDeviceHandle(const LibUSBDevice &device) {
	check_fn("libusb_open", libusb_open(device.device, &handle));
}

LibUSBDeviceHandle::LibUSBDeviceHandle(LibUSBContext &context, unsigned int vendor_id, unsigned int product_id) {
	LibUSBDeviceList lst(context);
	for (std::size_t i = 0; i < lst.size(); ++i) {
		const LibUSBDevice &device = lst[i];
		if (device.vendor_id() == vendor_id && device.product_id() == product_id) {
			for (std::size_t j = i + 1; j < lst.size(); ++j) {
				const LibUSBDevice &device = lst[j];
				if (device.vendor_id() == vendor_id && device.product_id() == product_id) {
					throw std::runtime_error("Two dongles attached");
				}
			}

			check_fn("libusb_open", libusb_open(device.device, &handle));
			return;
		}
	}

	throw std::runtime_error("No dongle attached");
}

LibUSBDeviceHandle::~LibUSBDeviceHandle() {
	libusb_close(handle);
}

int LibUSBDeviceHandle::get_configuration() const {
	int conf;
	check_fn("libusb_get_configuration", libusb_get_configuration(handle, &conf));
	return conf;
}

void LibUSBDeviceHandle::set_configuration(int config) {
	check_fn("libusb_set_configuration", libusb_set_configuration(handle, config));
}

void LibUSBDeviceHandle::claim_interface(int interface) {
	check_fn("libusb_claim_interface", libusb_claim_interface(handle, interface));
}

void LibUSBDeviceHandle::control_no_data(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, unsigned int timeout) {
	assert((request_type & LIBUSB_ENDPOINT_DIR_MASK) == 0);
	check_fn("libusb_control_transfer", libusb_control_transfer(handle, request_type | LIBUSB_ENDPOINT_OUT, request, value, index, 0, 0, timeout));
}

std::size_t LibUSBDeviceHandle::control_in(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, void *buffer, std::size_t len, unsigned int timeout) {
	assert((request_type & LIBUSB_ENDPOINT_DIR_MASK) == 0);
	assert(len < 65536);
	return check_fn("libusb_control_transfer", libusb_control_transfer(handle, request_type | LIBUSB_ENDPOINT_IN, request, value, index, static_cast<unsigned char *>(buffer), static_cast<uint16_t>(len), timeout));
}

std::size_t LibUSBDeviceHandle::interrupt_in(unsigned char endpoint, void *data, std::size_t length, unsigned int timeout, unsigned int stall_max) {
	assert((endpoint & LIBUSB_ENDPOINT_ADDRESS_MASK) == endpoint);
	assert(length < static_cast<std::size_t>(std::numeric_limits<int>::max()));
	int transferred = -1, rc;
	do {
		rc = libusb_interrupt_transfer(handle, endpoint | LIBUSB_ENDPOINT_IN, static_cast<unsigned char *>(data), static_cast<int>(length), &transferred, timeout);
		if (rc == LIBUSB_ERROR_PIPE && stall_max) {
			LOG_ERROR(Glib::ustring::compose("Ignored stall on IN endpoint %1", static_cast<unsigned int>(endpoint)));
		}
	} while (rc == LIBUSB_ERROR_PIPE && stall_max--);
	check_fn("libusb_interrupt_transfer", rc);
	assert(transferred >= 0);
	return transferred;
}



void LibUSBTransfer::result() const {
	assert(done_);
	switch (transfer->status) {
		case LIBUSB_TRANSFER_COMPLETED:
			return;

		case LIBUSB_TRANSFER_ERROR:
			throw LibUSBTransferError(transfer->endpoint, "Transfer error");

		case LIBUSB_TRANSFER_TIMED_OUT:
			throw LibUSBTransferTimeoutError(transfer->endpoint);

		case LIBUSB_TRANSFER_CANCELLED:
			throw LibUSBTransferCancelledError(transfer->endpoint);

		case LIBUSB_TRANSFER_STALL:
			throw LibUSBTransferStallError(transfer->endpoint);

		case LIBUSB_TRANSFER_NO_DEVICE:
			throw LibUSBTransferError(transfer->endpoint, "Device was disconnected");

		case LIBUSB_TRANSFER_OVERFLOW:
			throw LibUSBTransferError(transfer->endpoint, "Device sent more data than requested");

		default:
			throw ErrorMessageError();
	}
}

void LibUSBTransfer::submit() {
	assert(!submitted_);
	check_fn("libusb_submit_transfer", libusb_submit_transfer(transfer));
	submitted_ = true;
	done_ = false;
	submitted_self_ref.reset(this);
}

void LibUSBTransfer::trampoline(libusb_transfer *transfer) {
	LibUSBTransfer *ptr = static_cast<LibUSBTransfer *>(transfer->user_data);
	Glib::signal_idle().connect_once(sigc::mem_fun(ptr, &LibUSBTransfer::callback));
}

LibUSBTransfer::LibUSBTransfer(unsigned int stall_max) : transfer(libusb_alloc_transfer(0)), submitted_(false), done_(false), repeats_(false), stall_count(0), stall_max(stall_max) {
}

LibUSBTransfer::~LibUSBTransfer() {
	libusb_free_transfer(transfer);
}

void LibUSBTransfer::callback() {
	if ((transfer->status == LIBUSB_TRANSFER_STALL || transfer->status == LIBUSB_TRANSFER_ERROR) && stall_count < stall_max) {
		++stall_count;
		if (transfer->status == LIBUSB_TRANSFER_STALL) {
			LOG_ERROR(Glib::ustring::compose("Ignored stall %1 of %2 on %3 endpoint %4", stall_count, stall_max, ((transfer->endpoint & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN) ? "IN" : "OUT", static_cast<unsigned int>(transfer->endpoint & LIBUSB_ENDPOINT_ADDRESS_MASK)));
		} else {
			LOG_ERROR(Glib::ustring::compose("Ignored transfer error %1 of %2 on %3 endpoint %4", stall_count, stall_max, ((transfer->endpoint & LIBUSB_ENDPOINT_DIR_MASK) == LIBUSB_ENDPOINT_IN) ? "IN" : "OUT", static_cast<unsigned int>(transfer->endpoint & LIBUSB_ENDPOINT_ADDRESS_MASK)));
		}
		check_fn("libusb_submit_transfer", libusb_submit_transfer(transfer));
		return;
	} else if (transfer->status != LIBUSB_TRANSFER_STALL && transfer->status != LIBUSB_TRANSFER_ERROR) {
		stall_count = 0;
	}
	done_ = true;
	submitted_ = false;
	Ptr pthis(this);
	submitted_self_ref.reset();
	signal_done.emit(pthis);
	if (repeats_) {
		submit();
	}
}



LibUSBInterruptInTransfer::Ptr LibUSBInterruptInTransfer::create(LibUSBDeviceHandle &dev, unsigned char endpoint, std::size_t len, bool exact_len, unsigned int timeout, unsigned int stall_max) {
	Ptr p(new LibUSBInterruptInTransfer(dev, endpoint, len, exact_len, timeout, stall_max));
	return p;
}

LibUSBInterruptInTransfer::LibUSBInterruptInTransfer(LibUSBDeviceHandle &dev, unsigned char endpoint, std::size_t len, bool exact_len, unsigned int timeout, unsigned int stall_max) : LibUSBTransfer(stall_max) {
	assert((endpoint & LIBUSB_ENDPOINT_ADDRESS_MASK) == endpoint);
	libusb_fill_interrupt_transfer(transfer, dev.handle, endpoint | LIBUSB_ENDPOINT_IN, static_cast<unsigned char *>(std::malloc(len)), static_cast<int>(len), &LibUSBTransfer::trampoline, this, timeout);
	transfer->flags |= LIBUSB_TRANSFER_FREE_BUFFER;
	if (exact_len) {
		transfer->flags |= LIBUSB_TRANSFER_SHORT_NOT_OK;
	}
}



LibUSBInterruptOutTransfer::Ptr LibUSBInterruptOutTransfer::create(LibUSBDeviceHandle &dev, unsigned char endpoint, const void *data, std::size_t len, unsigned int timeout, unsigned int stall_max) {
	Ptr p(new LibUSBInterruptOutTransfer(dev, endpoint, data, len, timeout, stall_max));
	return p;
}

LibUSBInterruptOutTransfer::LibUSBInterruptOutTransfer(LibUSBDeviceHandle &dev, unsigned char endpoint, const void *data, std::size_t len, unsigned int timeout, unsigned int stall_max) : LibUSBTransfer(stall_max) {
	assert((endpoint & LIBUSB_ENDPOINT_ADDRESS_MASK) == endpoint);
	libusb_fill_interrupt_transfer(transfer, dev.handle, endpoint | LIBUSB_ENDPOINT_OUT, static_cast<unsigned char *>(std::malloc(len)), static_cast<int>(len), &LibUSBTransfer::trampoline, this, timeout);
	std::memcpy(transfer->buffer, data, len);
	transfer->flags |= LIBUSB_TRANSFER_FREE_BUFFER;
}

