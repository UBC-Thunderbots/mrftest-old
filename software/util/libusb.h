#ifndef UTIL_LIBUSB_H
#define UTIL_LIBUSB_H

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

namespace USB {
	/**
	 * \brief An error that occurs in a libusb library function.
	 */
	class Error : public std::runtime_error {
		public:
			Error(const std::string &msg);
	};

	/**
	 * \brief An error that occurs on a USB transfer.
	 */
	class TransferError : public Error {
		public:
			TransferError(unsigned int endpoint, const std::string &msg);
	};

	/**
	 * \brief An error that occurs when a USB transfer times out.
	 */
	class TransferTimeoutError : public TransferError {
		public:
			TransferTimeoutError(unsigned int endpoint);
	};

	/**
	 * \brief An error that occurs when a USB stall occurs.
	 */
	class TransferStallError : public TransferError {
		public:
			TransferStallError(unsigned int endpoint);
	};

	/**
	 * \brief An error that occurs when a USB transfer is cancelled.
	 */
	class TransferCancelledError : public TransferError {
		public:
			TransferCancelledError(unsigned int endpoint);
	};

	/**
	 * \brief A libusb context.
	 */
	class Context : public NonCopyable {
		public:
			Context();

			~Context();

		private:
			friend class DeviceList;
			friend class DeviceHandle;

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
	class Device {
		public:
			Device(const Device &copyref);

			~Device();

			Device &operator=(const Device &assgref);

			unsigned int vendor_id() const {
				return device_descriptor.idVendor;
			}

			unsigned int product_id() const {
				return device_descriptor.idProduct;
			}

			std::string serial_number() const;

		private:
			friend class DeviceList;
			friend class DeviceHandle;

			libusb_context *context;
			libusb_device *device;
			libusb_device_descriptor device_descriptor;

			Device(libusb_device *device);
	};

	/**
	 * \brief A list of USB devices.
	 */
	class DeviceList : public NonCopyable {
		public:
			DeviceList(Context &context);

			~DeviceList();

			std::size_t size() const {
				return size_;
			}

			Device operator[](const std::size_t i) const;

		private:
			libusb_context *context;
			std::size_t size_;
			libusb_device **devices;
	};

	/**
	 * \brief A libusb device handle.
	 */
	class DeviceHandle : public NonCopyable {
		public:
			DeviceHandle(Context &context, unsigned int vendor_id, unsigned int product_id, const char *serial_number = 0);

			DeviceHandle(const Device &device);

			~DeviceHandle();

			void reset();

			std::string get_string_descriptor(uint8_t index) const;

			int get_configuration() const;

			void set_configuration(int config);

			void claim_interface(int interface);

			void release_interface(int interface);

			void set_interface_alt_setting(int interface, int alternate_setting);

			void clear_halt_in(unsigned char endpoint);

			void clear_halt_out(unsigned char endpoint);

			void control_no_data(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, unsigned int timeout);

			std::size_t control_in(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, void *buffer, std::size_t len, unsigned int timeout);

			void control_out(uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, const void *buffer, std::size_t len, unsigned int timeout);

			std::size_t interrupt_in(unsigned char endpoint, void *data, std::size_t length, unsigned int timeout);

			void interrupt_out(unsigned char endpoint, const void *data, std::size_t length, unsigned int timeout);

			std::size_t bulk_in(unsigned char endpoint, void *data, std::size_t length, unsigned int timeout);

		private:
			friend class Transfer;
			friend class ControlNoDataTransfer;
			friend class InterruptOutTransfer;
			friend class InterruptInTransfer;
			friend class BulkOutTransfer;
			friend class BulkInTransfer;

			libusb_context *context;
			libusb_device_handle *handle;
			unsigned int submitted_transfer_count;
	};

	/**
	 * \brief An RAII-style way to set a configuration on a device.
	 */
	class ConfigurationSetter : public NonCopyable {
		public:
			/**
			 * \brief Sets a device’s configuration.
			 *
			 * \param[in] device the device to set
			 *
			 * \param[in] configuration the configuration number to set
			 */
			ConfigurationSetter(DeviceHandle &device, int configuration);

			/**
			 * \brief Resets the device to its original configuration.
			 */
			~ConfigurationSetter();

		private:
			DeviceHandle &device;
			int original_configuration;
	};

	/**
	 * \brief An RAII-style way to claim an interface on a device.
	 */
	class InterfaceClaimer : public NonCopyable {
		public:
			/**
			 * \brief Claims an interface on a device.
			 *
			 * \param[in] device the device whose interface should be claimed
			 *
			 * \param[in] interface the interface to claim
			 */
			InterfaceClaimer(DeviceHandle &device, int interface);

			/**
			 * \brief Releases the interface.
			 */
			~InterfaceClaimer();

		private:
			DeviceHandle &device;
			int interface;
	};

	/**
	 * \brief A libusb transfer.
	 */
	class Transfer : public AsyncOperation<void> {
		public:
			virtual ~Transfer();

			void result() const;

			void submit();

		protected:
			DeviceHandle &device;
			libusb_transfer *transfer;
			bool submitted_, done_;

			static void handle_completed_transfer_trampoline(libusb_transfer *transfer);
			Transfer(DeviceHandle &dev);
			void handle_completed_transfer();
	};

	/**
	 * \brief A libusb control transfer with no data.
	 */
	class ControlNoDataTransfer : public Transfer {
		public:
			ControlNoDataTransfer(DeviceHandle &dev, uint8_t request_type, uint8_t request, uint16_t value, uint16_t index, unsigned int timeout);
	};

	/**
	 * \brief A libusb inbound interrupt transfer.
	 */
	class InterruptInTransfer : public Transfer {
		public:
			InterruptInTransfer(DeviceHandle &dev, unsigned char endpoint, std::size_t len, bool exact_len, unsigned int timeout);

			const uint8_t *data() const {
				assert(done_);
				return transfer->buffer;
			}

			std::size_t size() const {
				assert(done_);
				return static_cast<std::size_t>(transfer->actual_length);
			}
	};

	/**
	 * \brief A libusb outbound interrupt transfer.
	 */
	class InterruptOutTransfer : public Transfer {
		public:
			InterruptOutTransfer(DeviceHandle &dev, unsigned char endpoint, const void *data, std::size_t len, std::size_t max_len, unsigned int timeout);
	};

	/**
	 * \brief A libusb inbound bulk transfer.
	 */
	class BulkInTransfer : public Transfer {
		public:
			BulkInTransfer(DeviceHandle &dev, unsigned char endpoint, std::size_t len, bool exact_len, unsigned int timeout);

			const uint8_t *data() const {
				assert(done_);
				return transfer->buffer;
			}

			std::size_t size() const {
				assert(done_);
				return static_cast<std::size_t>(transfer->actual_length);
			}
	};

	/**
	 * \brief A libusb outbound bulk transfer.
	 */
	class BulkOutTransfer : public Transfer {
		public:
			BulkOutTransfer(DeviceHandle &dev, unsigned char endpoint, const void *data, std::size_t len, std::size_t max_len, unsigned int timeout);
	};
}

#endif

