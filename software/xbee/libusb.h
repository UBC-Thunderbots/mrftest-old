#ifndef XBEE_LIBUSB_H
#define XBEE_LIBUSB_H

#include "util/async_operation.h"
#include "util/noncopyable.h"
#include <cassert>
#include <cstddef>
#include <glib.h>
#include <libusb.h>
#include <list>
#include <memory>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <unordered_map>
#include <sigc++/connection.h>

/**
 * \brief An error that occurs in a LibUSB library function.
 */
class LibUSBError : public std::runtime_error {
	public:
		LibUSBError(const std::string &msg);

		int error_code() const;
};

/**
 * \brief An error that occurs on a USB transfer.
 */
class LibUSBTransferError : public LibUSBError {
	public:
		LibUSBTransferError(unsigned int endpoint, const std::string &msg);
};

/**
 * \brief An error that occurs when a USB transfer times out.
 */
class LibUSBTransferTimeoutError : public LibUSBTransferError {
	public:
		LibUSBTransferTimeoutError(unsigned int endpoint);
};

/**
 * \brief An error that occurs when a USB stall occurs.
 */
class LibUSBTransferStallError : public LibUSBTransferError {
	public:
		LibUSBTransferStallError(unsigned int endpoint);
};

/**
 * \brief An error that occurs when a USB transfer is cancelled
 */
class LibUSBTransferCancelledError : public LibUSBTransferError {
	public:
		LibUSBTransferCancelledError(unsigned int endpoint);
};

/**
 * \brief A LibUSB context.
 */
class LibUSBContext : public NonCopyable {
	public:
		LibUSBContext();

		~LibUSBContext();

	private:
		friend class LibUSBDeviceList;
		friend class LibUSBDeviceHandle;

		libusb_context *context;
		std::unordered_map<int, sigc::connection> fd_connections;

		static void pollfd_add_trampoline(int fd, short events, void *user_data);
		static void pollfd_remove_trampoline(int fd, void *user_data);
		void add_pollfd(int fd, short events);
		void remove_pollfd(int fd);
		void handle_usb_fds();
};

/**
 * \brief A USB device.
 */
class LibUSBDevice {
	public:
		LibUSBDevice(const LibUSBDevice &copyref);

		~LibUSBDevice();

		LibUSBDevice &operator=(const LibUSBDevice &assgref);

		unsigned int vendor_id() const {
			return device_descriptor.idVendor;
		}

		unsigned int product_id() const {
			return device_descriptor.idProduct;
		}

	private:
		friend class LibUSBDeviceList;
		friend class LibUSBDeviceHandle;

		libusb_context *context;
		libusb_device *device;
		libusb_device_descriptor device_descriptor;

		LibUSBDevice(libusb_device *device);
};

/**
 * \brief A list of USB devices.
 */
class LibUSBDeviceList : public NonCopyable {
	public:
		LibUSBDeviceList(LibUSBContext &context);

		~LibUSBDeviceList();

		std::size_t size() const {
			return size_;
		}

		LibUSBDevice operator[](const std::size_t i) const;

	private:
		libusb_context *context;
		std::size_t size_;
		libusb_device **devices;
};

/**
 * \brief A LibUSB device handle.
 */
class LibUSBDeviceHandle : public NonCopyable {
	public:
		LibUSBDeviceHandle(LibUSBContext &context, unsigned int vendor_id, unsigned int product_id);

		LibUSBDeviceHandle(const LibUSBDevice &device);

		~LibUSBDeviceHandle();

		int get_configuration() const;

		void set_configuration(int config);

		void claim_interface(int interface);

		void control_no_data(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, unsigned int timeout);

		std::size_t control_in(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, void *buffer, std::size_t len, unsigned int timeout);

		std::size_t interrupt_in(unsigned char endpoint, void *data, std::size_t length, unsigned int timeout, unsigned int stall_max);

	private:
		friend class LibUSBTransfer;
		friend class LibUSBInterruptOutTransfer;
		friend class LibUSBInterruptInTransfer;

		libusb_context *context;
		libusb_device_handle *handle;
		unsigned int submitted_transfer_count;
};

/**
 * \brief A LibUSB transfer.
 */
class LibUSBTransfer : public AsyncOperation<void> {
	public:
		virtual ~LibUSBTransfer();

		void result() const;

		void submit();

	protected:
		LibUSBDeviceHandle &device;
		libusb_transfer *transfer;
		bool submitted_, done_;
		unsigned int stall_count, stall_max;

		static void handle_completed_transfer_trampoline(libusb_transfer *transfer);
		LibUSBTransfer(LibUSBDeviceHandle &dev, unsigned int stall_max);
		void handle_completed_transfer();
};

/**
 * \brief A LibUSB inbound interrupt transfer.
 */
class LibUSBInterruptInTransfer : public LibUSBTransfer {
	public:
		LibUSBInterruptInTransfer(LibUSBDeviceHandle &dev, unsigned char endpoint, std::size_t len, bool exact_len, unsigned int timeout, unsigned int stall_max);

		const uint8_t *data() const {
			assert(done_);
			return transfer->buffer;
		}

		std::size_t size() const {
			assert(done_);
			return transfer->actual_length;
		}
};

/**
 * \brief A LibUSB outbound interrupt transfer.
 */
class LibUSBInterruptOutTransfer : public LibUSBTransfer {
	public:
		LibUSBInterruptOutTransfer(LibUSBDeviceHandle &dev, unsigned char endpoint, const void *data, std::size_t len, unsigned int timeout, unsigned int stall_max);
};

#endif

