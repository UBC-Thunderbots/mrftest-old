#include "xbee/libusb.h"
#include "util/exception.h"
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <poll.h>
#include <sstream>

#warning needs Doxygen

namespace {
	int check_fn(const char *call, int err) {
		if (err >= 0) {
			return err;
		}

		const char *msg;
		switch (err) {
			case LIBUSB_ERROR_IO: msg = "Input/output error"; break;
			case LIBUSB_ERROR_INVALID_PARAM: msg = "Invalid parameter"; break;
			case LIBUSB_ERROR_ACCESS: msg = "Access denied (insufficient permissions)"; break;
			case LIBUSB_ERROR_NO_DEVICE: msg = "No such device (it may have been disconnected)"; break;
			case LIBUSB_ERROR_NOT_FOUND: msg = "Entity not found"; break;
			case LIBUSB_ERROR_BUSY: msg = "Resource busy"; break;
			case LIBUSB_ERROR_TIMEOUT: msg = "Operation timed out"; break;
			case LIBUSB_ERROR_OVERFLOW: msg = "Overflow"; break;
			case LIBUSB_ERROR_PIPE: msg = "Pipe error"; break;
			case LIBUSB_ERROR_INTERRUPTED: msg = "System call interrupted (perhaps due to signal)"; break;
			case LIBUSB_ERROR_NO_MEM: throw std::bad_alloc(); break;
			case LIBUSB_ERROR_NOT_SUPPORTED: msg = "Operation not supported or unimplemented on this platform"; break;
			case LIBUSB_ERROR_OTHER: msg = "Other error"; break;
			default: throw ErrorMessageError();
		}
		std::ostringstream oss;
		oss << call << ": " << msg;
		throw LibUSBError(oss.str());
	}

	ssize_t check_fn(const char *call, ssize_t ssz) {
		if (ssz < 0) {
			check_fn(call, static_cast<int>(ssz));
		}
		return ssz;
	}

	std::string make_transfer_error_message(unsigned int endpoint, const std::string &msg) {
		std::ostringstream oss;
		oss << msg << " on " << ((endpoint & 0x80) ? "IN" : "OUT") << " endpoint " << (endpoint & 0x7F);
		return oss.str();
	}

	class SyncResultAsyncOperation : public AsyncOperation<void> {
		public:
			static Ptr create(const char *call, int err) {
				Ptr p(new SyncResultAsyncOperation(call, err));
				return p;
			}

			void result() const {
				check_fn(call, err);
			}

		private:
			const char * const call;
			const int err;

			SyncResultAsyncOperation(const char *call, int err) : call(call), err(err) {
				Glib::signal_idle().connect_once(sigc::bind(signal_done.make_slot(), Ptr(this)));
			}

			~SyncResultAsyncOperation() {
			}
	};
}

LibUSBError::LibUSBError(const std::string &msg) : std::runtime_error(msg) {
}

LibUSBError::~LibUSBError() throw() {
}

LibUSBTransferError::LibUSBTransferError(unsigned int endpoint, const std::string &msg) : LibUSBError(make_transfer_error_message(endpoint, msg)) {
}

LibUSBTransferError::~LibUSBTransferError() throw() {
}

LibUSBTransferTimeoutError::LibUSBTransferTimeoutError(unsigned int endpoint) : LibUSBTransferError(endpoint, "Transfer timed out") {
}

LibUSBTransferTimeoutError::~LibUSBTransferTimeoutError() throw() {
}

LibUSBTransferStallError::LibUSBTransferStallError(unsigned int endpoint) : LibUSBTransferError(endpoint, "Transfer stalled") {
}

LibUSBTransferStallError::~LibUSBTransferStallError() throw() {
}

LibUSBTransferCancelledError::LibUSBTransferCancelledError(unsigned int endpoint) : LibUSBTransferError(endpoint, "Transfer cancelled") {
}

LibUSBTransferCancelledError::~LibUSBTransferCancelledError() throw() {
}

LibUSBContext::LibUSBContext() : destroyed_flag(0) {
	check_fn("libusb_init", libusb_init(&context));
	instances.push_front(this);
	instances_iter = instances.begin();
	assert(*instances_iter == this);
	Glib::MainContext::get_default()->set_poll_func(&LibUSBContext::poll_func);
}

LibUSBContext::~LibUSBContext() {
	libusb_exit(context);
	instances.erase(instances_iter);
	if (destroyed_flag) {
		*destroyed_flag = true;
	}
}

std::list<LibUSBContext *> LibUSBContext::instances;

int LibUSBContext::poll_func(GPollFD *ufds, unsigned int nfds, int timeout) {
	struct ContextInfo {
		LibUSBContext *context;
		const libusb_pollfd **fds;
		std::size_t nfds;
		bool destroyed;
	};

	ContextInfo context_infos[instances.size()];
	std::size_t total_usb_fds = 0;
	{
		std::size_t wptr = 0;
		for (std::list<LibUSBContext *>::const_iterator i = instances.begin(), iend = instances.end(); i != iend; ++i) {
			LibUSBContext *ctx = *i;
			context_infos[wptr].context = ctx;
			context_infos[wptr].fds = libusb_get_pollfds(ctx->context);
			for (context_infos[wptr].nfds = 0; context_infos[wptr].fds[context_infos[wptr].nfds]; ++context_infos[wptr].nfds);
			context_infos[wptr].destroyed = false;
			ctx->destroyed_flag = &context_infos[wptr].destroyed;
			total_usb_fds += context_infos[wptr].nfds;
			timeval tv;
			if (check_fn("libusb_get_next_timeout", libusb_get_next_timeout(ctx->context, &tv)) == 1) {
				unsigned long long millis = tv.tv_sec * 1000ULL + tv.tv_usec / 1000ULL;
				if (millis > std::numeric_limits<unsigned int>::max()) {
					millis = std::numeric_limits<unsigned int>::max();
				}
				if (timeout < 0 || millis < static_cast<unsigned int>(timeout)) {
					timeout = static_cast<int>(millis);
				}
			}
			++wptr;
		}
	}

	pollfd pfds[total_usb_fds + nfds];
	{
		std::size_t wptr = 0;
		for (std::size_t i = 0; i < G_N_ELEMENTS(context_infos); ++i) {
			for (std::size_t j = 0; j < context_infos[i].nfds; ++j) {
				pfds[wptr].fd = context_infos[i].fds[j]->fd;
				pfds[wptr].events = context_infos[i].fds[j]->events;
				++wptr;
			}
		}
		for (std::size_t i = 0; i < nfds; ++i) {
			pfds[wptr].fd = ufds[i].fd;
			pfds[wptr].events = ufds[i].events;
			++wptr;
		}
	}

	if (poll(pfds, G_N_ELEMENTS(pfds), timeout) < 0) {
		throw SystemError("poll", errno);
	}

	for (std::size_t i = 0; i < G_N_ELEMENTS(context_infos); ++i) {
		if (!context_infos[i].destroyed) {
			struct timeval zero = { 0, 0 };
			check_fn("libusb_handle_events_timeout", libusb_handle_events_timeout(context_infos[i].context->context, &zero));
		}
	}

	int rc = 0;
	for (std::size_t i = 0; i < nfds; ++i) {
		ufds[i].revents = pfds[total_usb_fds + i].revents;
		if (pfds[total_usb_fds + i].revents) {
			++rc;
		}
	}

	return rc;
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

AsyncOperation<void>::Ptr LibUSBDeviceHandle::set_configuration(int config) {
	return SyncResultAsyncOperation::create("libusb_set_configuration", libusb_set_configuration(handle, config));
}

AsyncOperation<void>::Ptr LibUSBDeviceHandle::claim_interface(int interface) {
	return SyncResultAsyncOperation::create("libusb_claim_interface", libusb_claim_interface(handle, interface));
}

AsyncOperation<void>::Ptr LibUSBDeviceHandle::set_interface_alt_setting(int interface, int setting) {
	return SyncResultAsyncOperation::create("libusb_set_interface_alt_setting", libusb_set_interface_alt_setting(handle, interface, setting));
}

AsyncOperation<void>::Ptr LibUSBDeviceHandle::clear_halt(unsigned char endpoint) {
	return SyncResultAsyncOperation::create("libusb_clear_halt", libusb_clear_halt(handle, endpoint));
}

void LibUSBTransfer::result() const {
	assert(done_);
	switch (transfer->status) {
		case LIBUSB_TRANSFER_COMPLETED: return;
		case LIBUSB_TRANSFER_ERROR: throw LibUSBTransferError(transfer->endpoint, "Transfer error");
		case LIBUSB_TRANSFER_TIMED_OUT: throw LibUSBTransferTimeoutError(transfer->endpoint);
		case LIBUSB_TRANSFER_CANCELLED: throw LibUSBTransferCancelledError(transfer->endpoint);
		case LIBUSB_TRANSFER_STALL: throw LibUSBTransferStallError(transfer->endpoint);
		case LIBUSB_TRANSFER_NO_DEVICE: throw LibUSBTransferError(transfer->endpoint, "Device was disconnected");
		case LIBUSB_TRANSFER_OVERFLOW: throw LibUSBTransferError(transfer->endpoint, "Device sent more data than requested");
		default: throw ErrorMessageError();
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
	ptr->callback();
}

LibUSBTransfer::LibUSBTransfer(unsigned int stall_max) : transfer(libusb_alloc_transfer(0)), submitted_(false), done_(false), repeats_(false), stall_count(0), stall_max(stall_max) {
}

LibUSBTransfer::~LibUSBTransfer() {
	libusb_free_transfer(transfer);
}

void LibUSBTransfer::callback() {
	if (transfer->status == LIBUSB_TRANSFER_STALL && stall_count < stall_max) {
		++stall_count;
		check_fn("libusb_submit_transfer", libusb_submit_transfer(transfer));
		return;
	} else if (transfer->status != LIBUSB_TRANSFER_STALL) {
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

LibUSBControlNoDataTransfer::Ptr LibUSBControlNoDataTransfer::create(LibUSBDeviceHandle &dev, uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, unsigned int timeout, unsigned int stall_max) {
	Ptr p(new LibUSBControlNoDataTransfer(dev, request_type, request, value, index, timeout, stall_max));
	return p;
}

LibUSBControlNoDataTransfer::~LibUSBControlNoDataTransfer() {
}

LibUSBControlNoDataTransfer::LibUSBControlNoDataTransfer(LibUSBDeviceHandle &dev, uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, unsigned int timeout, unsigned int stall_max) : LibUSBTransfer(stall_max) {
	libusb_fill_control_setup(buffer, request_type, request, value, index, 0);
	libusb_fill_control_transfer(transfer, dev.handle, buffer, &LibUSBTransfer::trampoline, this, timeout);
}

LibUSBInterruptInTransfer::Ptr LibUSBInterruptInTransfer::create(LibUSBDeviceHandle &dev, unsigned char endpoint, std::size_t len, bool exact_len, unsigned int timeout, unsigned int stall_max) {
	Ptr p(new LibUSBInterruptInTransfer(dev, endpoint, len, exact_len, timeout, stall_max));
	return p;
}

LibUSBInterruptInTransfer::~LibUSBInterruptInTransfer() {
}

LibUSBInterruptInTransfer::LibUSBInterruptInTransfer(LibUSBDeviceHandle &dev, unsigned char endpoint, std::size_t len, bool exact_len, unsigned int timeout, unsigned int stall_max) : LibUSBTransfer(stall_max) {
	libusb_fill_interrupt_transfer(transfer, dev.handle, endpoint | 0x80, static_cast<unsigned char *>(std::malloc(len)), static_cast<int>(len), &LibUSBTransfer::trampoline, this, timeout);
	transfer->flags |= LIBUSB_TRANSFER_FREE_BUFFER;
	if (exact_len) {
		transfer->flags |= LIBUSB_TRANSFER_SHORT_NOT_OK;
	}
}

LibUSBInterruptOutTransfer::Ptr LibUSBInterruptOutTransfer::create(LibUSBDeviceHandle &dev, unsigned char endpoint, const void *data, std::size_t len, unsigned int timeout, unsigned int stall_max) {
	Ptr p(new LibUSBInterruptOutTransfer(dev, endpoint, data, len, timeout, stall_max));
	return p;
}

LibUSBInterruptOutTransfer::~LibUSBInterruptOutTransfer() {
}

LibUSBInterruptOutTransfer::LibUSBInterruptOutTransfer(LibUSBDeviceHandle &dev, unsigned char endpoint, const void *data, std::size_t len, unsigned int timeout, unsigned int stall_max) : LibUSBTransfer(stall_max) {
	libusb_fill_interrupt_transfer(transfer, dev.handle, endpoint & 0x7F, static_cast<unsigned char *>(std::malloc(len)), static_cast<int>(len), &LibUSBTransfer::trampoline, this, timeout);
	std::memcpy(transfer->buffer, data, len);
	transfer->flags |= LIBUSB_TRANSFER_FREE_BUFFER;
}

