#include "xbee/robot.h"
#include "xbee/dongle.h"
#include <cassert>
#include <cstring>

#warning Doxygen

namespace {
	enum TBotsFirmwareRequest {
		FIRMWARE_REQUEST_CHIP_ERASE,
		FIRMWARE_REQUEST_FILL_PAGE_BUFFER,
		FIRMWARE_REQUEST_PAGE_PROGRAM,
		FIRMWARE_REQUEST_CRC_BLOCK,
		FIRMWARE_REQUEST_READ_PARAMS,
		FIRMWARE_REQUEST_SET_PARAMS,
		FIRMWARE_REQUEST_ROLLBACK_PARAMS,
		FIRMWARE_REQUEST_COMMIT_PARAMS,
		FIRMWARE_REQUEST_REBOOT,
		FIRMWARE_REQUEST_READ_BUILD_SIGNATURES,
	};

	class FirmwareSPIChipEraseOperation : public AsyncOperation<void>, public sigc::trackable {
		public:
			static Ptr create(XBeeDongle &dongle, unsigned int robot) {
				Ptr p(new FirmwareSPIChipEraseOperation(dongle, robot));
				return p;
			}

			void result() const {
				if (failed_operation.is()) {
					failed_operation->result();
				}
			}

		private:
			XBeeDongle &dongle;
			const unsigned int robot;
			AsyncOperation<void>::Ptr failed_operation;
			sigc::connection send_connection, receive_connection;
			Ptr self_ref;

			FirmwareSPIChipEraseOperation(XBeeDongle &dongle, unsigned int robot) : dongle(dongle), robot(robot), self_ref(this) {
				const uint8_t data[] = { static_cast<uint8_t>((robot << 4) | XBeeDongle::PIPE_FIRMWARE_OUT), FIRMWARE_REQUEST_CHIP_ERASE };
				send_connection = dongle.send_bulk(data, sizeof(data))->signal_done.connect(sigc::mem_fun(this, &FirmwareSPIChipEraseOperation::on_send_bulk_done));
				receive_connection = dongle.signal_interrupt_message_received.connect(sigc::mem_fun(this, &FirmwareSPIChipEraseOperation::on_interrupt_message_received));
			}

			void on_send_bulk_done(AsyncOperation<void>::Ptr op) {
				if (!op->succeeded()) {
					receive_connection.disconnect();
					failed_operation = op;
					Ptr pthis(this);
					self_ref.reset();
					signal_done.emit(pthis);
				}
			}

			void on_interrupt_message_received(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len) {
				if (robot == this->robot && pipe == XBeeDongle::PIPE_FIRMWARE_IN) {
					if (len == 1 && static_cast<const uint8_t *>(data)[0] == FIRMWARE_REQUEST_CHIP_ERASE) {
						send_connection.disconnect();
						receive_connection.disconnect();
						Ptr pthis(this);
						self_ref.reset();
						signal_done.emit(pthis);
					} else {
#warning TODO something sensible
					}
				}
			}
	};

	class FirmwareSPIFillPageBufferOperation : public AsyncOperation<void>, public sigc::trackable {
		public:
			static Ptr create(XBeeDongle &dongle, unsigned int robot, unsigned int offset, const void *data, std::size_t length) {
				Ptr p(new FirmwareSPIFillPageBufferOperation(dongle, robot, offset, data, length));
				return p;
			}

			void result() const {
				if (failed_operation.is()) {
					failed_operation->result();
				}
			}

		private:
			XBeeDongle &dongle;
			const unsigned int robot;
			AsyncOperation<void>::Ptr failed_operation;
			Ptr self_ref;

			FirmwareSPIFillPageBufferOperation(XBeeDongle &dongle, unsigned int robot, unsigned int offset, const void *data, std::size_t length) : dongle(dongle), robot(robot), self_ref(this) {
				uint8_t buffer[length + 3];
				assert(sizeof(buffer) <= 64);
				assert(offset + length <= 256);
				buffer[0] = static_cast<uint8_t>((robot << 4) | XBeeDongle::PIPE_FIRMWARE_OUT);
				buffer[1] = FIRMWARE_REQUEST_FILL_PAGE_BUFFER;
				buffer[2] = static_cast<uint8_t>(offset);
				std::memcpy(buffer + 3, data, length);
				dongle.send_bulk(buffer, sizeof(buffer))->signal_done.connect(sigc::mem_fun(this, &FirmwareSPIFillPageBufferOperation::on_send_bulk_done));
			}

			void on_send_bulk_done(AsyncOperation<void>::Ptr op) {
				if (!op->succeeded()) {
					failed_operation = op;
				}
				Ptr pthis(this);
				self_ref.reset();
				signal_done.emit(pthis);
			}
	};

	class FirmwareSPIPageProgramOperation : public AsyncOperation<void>, public sigc::trackable {
		public:
			static Ptr create(XBeeDongle &dongle, unsigned int robot, unsigned int page, uint16_t crc) {
				Ptr p(new FirmwareSPIPageProgramOperation(dongle, robot, page, crc));
				return p;
			}

			void result() const {
				if (failed_operation.is()) {
					failed_operation->result();
				}
			}

		private:
			const unsigned int robot;
			AsyncOperation<void>::Ptr failed_operation;
			sigc::connection send_connection, receive_connection;
			Ptr self_ref;

			FirmwareSPIPageProgramOperation(XBeeDongle &dongle, unsigned int robot, unsigned int page, uint16_t crc) : robot(robot), self_ref(this) {
				uint8_t buffer[6];
				buffer[0] = static_cast<uint8_t>((robot << 4) | XBeeDongle::PIPE_FIRMWARE_OUT);
				buffer[1] = FIRMWARE_REQUEST_PAGE_PROGRAM;
				buffer[2] = static_cast<uint8_t>(page);
				buffer[3] = static_cast<uint8_t>(page >> 8);
				buffer[4] = static_cast<uint8_t>(crc);
				buffer[5] = static_cast<uint8_t>(crc >> 8);
				send_connection = dongle.send_bulk(buffer, sizeof(buffer))->signal_done.connect(sigc::mem_fun(this, &FirmwareSPIPageProgramOperation::on_send_bulk_done));
				receive_connection = dongle.signal_interrupt_message_received.connect(sigc::mem_fun(this, &FirmwareSPIPageProgramOperation::on_interrupt_message_received));
			}

			void on_send_bulk_done(AsyncOperation<void>::Ptr op) {
				if (!op->succeeded()) {
					receive_connection.disconnect();
					failed_operation = op;
					Ptr pthis(this);
					self_ref.reset();
					signal_done.emit(pthis);
				}
			}

			void on_interrupt_message_received(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len) {
				if (robot == this->robot && pipe == XBeeDongle::PIPE_FIRMWARE_IN) {
					if (len == 1 && static_cast<const uint8_t *>(data)[0] == FIRMWARE_REQUEST_PAGE_PROGRAM) {
						send_connection.disconnect();
						receive_connection.disconnect();
						Ptr pthis(this);
						self_ref.reset();
						signal_done.emit(pthis);
					} else {
#warning TODO something sensible
					}
				}
			}
	};

	class FirmwareSPIBlockCRCOperation : public AsyncOperation<uint16_t>, public sigc::trackable {
		public:
			static Ptr create(XBeeDongle &dongle, unsigned int robot, unsigned int address, std::size_t length) {
				Ptr p(new FirmwareSPIBlockCRCOperation(dongle, robot, address, length));
				return p;
			}

			uint16_t result() const {
				if (failed_operation.is()) {
					failed_operation->result();
				}
				return crc;
			}

		private:
			XBeeDongle &dongle;
			const unsigned int robot;
			const unsigned int address;
			const std::size_t length;
			AsyncOperation<void>::Ptr failed_operation;
			sigc::connection send_connection, receive_connection;
			Ptr self_ref;
			uint16_t crc;

			FirmwareSPIBlockCRCOperation(XBeeDongle &dongle, unsigned int robot, unsigned int address, std::size_t length) : dongle(dongle), robot(robot), address(address), length(length), self_ref(this) {
				assert(1 <= length && length <= 65536);
				uint8_t data[7];
				data[0] = static_cast<uint8_t>((robot << 4) | XBeeDongle::PIPE_FIRMWARE_OUT);
				data[1] = FIRMWARE_REQUEST_CRC_BLOCK;
				data[2] = static_cast<uint8_t>(address);
				data[3] = static_cast<uint8_t>(address >> 8);
				data[4] = static_cast<uint8_t>(address >> 16);
				data[5] = static_cast<uint8_t>(length);
				data[6] = static_cast<uint8_t>(length >> 8);
				send_connection = dongle.send_bulk(data, sizeof(data))->signal_done.connect(sigc::mem_fun(this, &FirmwareSPIBlockCRCOperation::on_send_bulk_done));
				receive_connection = dongle.signal_interrupt_message_received.connect(sigc::mem_fun(this, &FirmwareSPIBlockCRCOperation::on_interrupt_message_received));
			}

			void on_send_bulk_done(AsyncOperation<void>::Ptr op) {
				if (!op->succeeded()) {
					receive_connection.disconnect();
					failed_operation = op;
					Ptr pthis(this);
					self_ref.reset();
					signal_done.emit(pthis);
				}
			}

			void on_interrupt_message_received(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len) {
				if (robot == this->robot && pipe == XBeeDongle::PIPE_FIRMWARE_IN) {
					const uint8_t *pch = static_cast<const uint8_t *>(data);
					if (len == 8 && pch[0] == FIRMWARE_REQUEST_CRC_BLOCK && static_cast<uint32_t>(pch[1] | (pch[2] << 8) | (pch[3] << 16)) == address && static_cast<uint16_t>(pch[4] | (pch[5] << 8)) == (length & 0xFFFF)) {
						crc = static_cast<uint16_t>(pch[6] | (pch[7] << 8));
						send_connection.disconnect();
						receive_connection.disconnect();
						Ptr pthis(this);
						self_ref.reset();
						signal_done.emit(pthis);
					} else {
#warning TODO something sensible
					}
				}
			}
	};

	class FirmwareReadOperationalParametersOperation : public AsyncOperation<XBeeRobot::OperationalParameters>, public sigc::trackable {
		public:
			static Ptr create(XBeeDongle &dongle, unsigned int robot) {
				Ptr p(new FirmwareReadOperationalParametersOperation(dongle, robot));
				return p;
			}

			XBeeRobot::OperationalParameters result() const {
				if (failed_operation.is()) {
					failed_operation->result();
				}
				return params;
			}

		private:
			XBeeDongle &dongle;
			const unsigned int robot;
			AsyncOperation<void>::Ptr failed_operation;
			sigc::connection send_connection, receive_connection;
			Ptr self_ref;
			XBeeRobot::OperationalParameters params;

			FirmwareReadOperationalParametersOperation(XBeeDongle &dongle, unsigned int robot) : dongle(dongle), robot(robot), self_ref(this) {
				uint8_t data[2];
				data[0] = static_cast<uint8_t>((robot << 4) | XBeeDongle::PIPE_FIRMWARE_OUT);
				data[1] = FIRMWARE_REQUEST_READ_PARAMS;
				send_connection = dongle.send_bulk(data, sizeof(data))->signal_done.connect(sigc::mem_fun(this, &FirmwareReadOperationalParametersOperation::on_send_bulk_done));
				receive_connection = dongle.signal_interrupt_message_received.connect(sigc::mem_fun(this, &FirmwareReadOperationalParametersOperation::on_interrupt_message_received));
			}

			void on_send_bulk_done(AsyncOperation<void>::Ptr op) {
				if (!op->succeeded()) {
					receive_connection.disconnect();
					failed_operation = op;
					Ptr pthis(this);
					self_ref.reset();
					signal_done.emit(pthis);
				}
			}

			void on_interrupt_message_received(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len) {
				if (robot == this->robot && pipe == XBeeDongle::PIPE_FIRMWARE_IN) {
					const uint8_t *pch = static_cast<const uint8_t *>(data);
					if (len == 7 && pch[0] == FIRMWARE_REQUEST_READ_PARAMS) {
#warning sanity checks
						params.flash_contents = static_cast<XBeeRobot::OperationalParameters::FlashContents>(pch[1]);
						params.xbee_channels[0] = pch[2];
						params.xbee_channels[1] = pch[3];
						params.robot_number = pch[4];
						params.dribble_power = pch[5];
						send_connection.disconnect();
						receive_connection.disconnect();
						Ptr pthis(this);
						self_ref.reset();
						signal_done.emit(pthis);
					} else {
#warning TODO something sensible
					}
				}
			}
	};

	class FirmwareSetOperationalParametersOperation : public AsyncOperation<void>, public sigc::trackable {
		public:
			static Ptr create(XBeeDongle &dongle, unsigned int robot, const XBeeRobot::OperationalParameters &params) {
				Ptr p(new FirmwareSetOperationalParametersOperation(dongle, robot, params));
				return p;
			}

			void result() const {
				if (failed_operation.is()) {
					failed_operation->result();
				}
			}

		private:
			XBeeDongle &dongle;
			const unsigned int robot;
			AsyncOperation<void>::Ptr failed_operation;
			sigc::connection send_connection, receive_connection;
			Ptr self_ref;
			XBeeRobot::OperationalParameters params;

			FirmwareSetOperationalParametersOperation(XBeeDongle &dongle, unsigned int robot, const XBeeRobot::OperationalParameters &params) : dongle(dongle), robot(robot), self_ref(this) {
				uint8_t data[8];
#warning sanity checks
				data[0] = static_cast<uint8_t>((robot << 4) | XBeeDongle::PIPE_FIRMWARE_OUT);
				data[1] = FIRMWARE_REQUEST_SET_PARAMS;
				data[2] = params.flash_contents;
				data[3] = params.xbee_channels[0];
				data[4] = params.xbee_channels[1];
				data[5] = params.robot_number;
				data[6] = params.dribble_power;
				data[7] = 0;
				send_connection = dongle.send_bulk(data, sizeof(data))->signal_done.connect(sigc::mem_fun(this, &FirmwareSetOperationalParametersOperation::on_send_bulk_done));
				receive_connection = dongle.signal_interrupt_message_received.connect(sigc::mem_fun(this, &FirmwareSetOperationalParametersOperation::on_interrupt_message_received));
			}

			void on_send_bulk_done(AsyncOperation<void>::Ptr op) {
				if (!op->succeeded()) {
					receive_connection.disconnect();
					failed_operation = op;
					Ptr pthis(this);
					self_ref.reset();
					signal_done.emit(pthis);
				}
			}

			void on_interrupt_message_received(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len) {
				if (robot == this->robot && pipe == XBeeDongle::PIPE_FIRMWARE_IN) {
					const uint8_t *pch = static_cast<const uint8_t *>(data);
					if (len == 1 && pch[0] == FIRMWARE_REQUEST_SET_PARAMS) {
						send_connection.disconnect();
						receive_connection.disconnect();
						Ptr pthis(this);
						self_ref.reset();
						signal_done.emit(pthis);
					} else {
#warning TODO something sensible
					}
				}
			}
	};

	class FirmwareCommitOperationalParametersOperation : public AsyncOperation<void>, public sigc::trackable {
		public:
			static Ptr create(XBeeDongle &dongle, unsigned int robot) {
				Ptr p(new FirmwareCommitOperationalParametersOperation(dongle, robot));
				return p;
			}

			void result() const {
				if (failed_operation.is()) {
					failed_operation->result();
				}
			}

		private:
			XBeeDongle &dongle;
			const unsigned int robot;
			AsyncOperation<void>::Ptr failed_operation;
			sigc::connection send_connection, receive_connection;
			Ptr self_ref;
			XBeeRobot::OperationalParameters params;

			FirmwareCommitOperationalParametersOperation(XBeeDongle &dongle, unsigned int robot) : dongle(dongle), robot(robot), self_ref(this) {
				uint8_t data[2];
				data[0] = static_cast<uint8_t>((robot << 4) | XBeeDongle::PIPE_FIRMWARE_OUT);
				data[1] = FIRMWARE_REQUEST_COMMIT_PARAMS;
				send_connection = dongle.send_bulk(data, sizeof(data))->signal_done.connect(sigc::mem_fun(this, &FirmwareCommitOperationalParametersOperation::on_send_bulk_done));
				receive_connection = dongle.signal_interrupt_message_received.connect(sigc::mem_fun(this, &FirmwareCommitOperationalParametersOperation::on_interrupt_message_received));
			}

			void on_send_bulk_done(AsyncOperation<void>::Ptr op) {
				if (!op->succeeded()) {
					receive_connection.disconnect();
					failed_operation = op;
					Ptr pthis(this);
					self_ref.reset();
					signal_done.emit(pthis);
				}
			}

			void on_interrupt_message_received(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len) {
				if (robot == this->robot && pipe == XBeeDongle::PIPE_FIRMWARE_IN) {
					const uint8_t *pch = static_cast<const uint8_t *>(data);
					if (len == 1 && pch[0] == FIRMWARE_REQUEST_COMMIT_PARAMS) {
						send_connection.disconnect();
						receive_connection.disconnect();
						Ptr pthis(this);
						self_ref.reset();
						signal_done.emit(pthis);
					} else {
#warning TODO something sensible
					}
				}
			}
	};

	class FirmwareRebootOperation : public AsyncOperation<void>, public sigc::trackable {
		public:
			static Ptr create(XBeeDongle &dongle, unsigned int robot) {
				Ptr p(new FirmwareRebootOperation(dongle, robot));
				return p;
			}

			void result() const {
				if (failed_operation.is()) {
					failed_operation->result();
				}
			}

		private:
			XBeeDongle &dongle;
			const unsigned int robot;
			AsyncOperation<void>::Ptr failed_operation;
			sigc::connection send_connection, receive_connection;
			Ptr self_ref;
			XBeeRobot::OperationalParameters params;

			FirmwareRebootOperation(XBeeDongle &dongle, unsigned int robot) : dongle(dongle), robot(robot), self_ref(this) {
				uint8_t data[2];
				data[0] = static_cast<uint8_t>((robot << 4) | XBeeDongle::PIPE_FIRMWARE_OUT);
				data[1] = FIRMWARE_REQUEST_REBOOT;
				send_connection = dongle.send_bulk(data, sizeof(data))->signal_done.connect(sigc::mem_fun(this, &FirmwareRebootOperation::on_send_bulk_done));
				receive_connection = dongle.signal_interrupt_message_received.connect(sigc::mem_fun(this, &FirmwareRebootOperation::on_interrupt_message_received));
			}

			void on_send_bulk_done(AsyncOperation<void>::Ptr op) {
				if (!op->succeeded()) {
					receive_connection.disconnect();
					failed_operation = op;
					Ptr pthis(this);
					self_ref.reset();
					signal_done.emit(pthis);
				}
			}

			void on_interrupt_message_received(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len) {
				if (robot == this->robot && pipe == XBeeDongle::PIPE_FIRMWARE_IN) {
					const uint8_t *pch = static_cast<const uint8_t *>(data);
					if (len == 1 && pch[0] == FIRMWARE_REQUEST_REBOOT) {
						send_connection.disconnect();
						receive_connection.disconnect();
						Ptr pthis(this);
						self_ref.reset();
						signal_done.emit(pthis);
					} else {
#warning TODO something sensible
					}
				}
			}
	};

	class FirmwareReadBuildSignaturesOperation : public AsyncOperation<XBeeRobot::BuildSignatures>, public sigc::trackable {
		public:
			static Ptr create(XBeeDongle &dongle, unsigned int robot) {
				Ptr p(new FirmwareReadBuildSignaturesOperation(dongle, robot));
				return p;
			}

			XBeeRobot::BuildSignatures result() const {
				if (failed_operation.is()) {
					failed_operation->result();
				}
				return sigs;
			}

		private:
			XBeeDongle &dongle;
			const unsigned int robot;
			AsyncOperation<void>::Ptr failed_operation;
			sigc::connection send_connection, receive_connection;
			Ptr self_ref;
			XBeeRobot::BuildSignatures sigs;

			FirmwareReadBuildSignaturesOperation(XBeeDongle &dongle, unsigned int robot) : dongle(dongle), robot(robot), self_ref(this) {
				uint8_t data[2];
				data[0] = static_cast<uint8_t>((robot << 4) | XBeeDongle::PIPE_FIRMWARE_OUT);
				data[1] = FIRMWARE_REQUEST_READ_BUILD_SIGNATURES;
				send_connection = dongle.send_bulk(data, sizeof(data))->signal_done.connect(sigc::mem_fun(this, &FirmwareReadBuildSignaturesOperation::on_send_bulk_done));
				receive_connection = dongle.signal_interrupt_message_received.connect(sigc::mem_fun(this, &FirmwareReadBuildSignaturesOperation::on_interrupt_message_received));
			}

			void on_send_bulk_done(AsyncOperation<void>::Ptr op) {
				if (!op->succeeded()) {
					receive_connection.disconnect();
					failed_operation = op;
					Ptr pthis(this);
					self_ref.reset();
					signal_done.emit(pthis);
				}
			}

			void on_interrupt_message_received(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len) {
				if (robot == this->robot && pipe == XBeeDongle::PIPE_FIRMWARE_IN) {
					const uint8_t *pch = static_cast<const uint8_t *>(data);
					if (len == 5 && pch[0] == FIRMWARE_REQUEST_READ_BUILD_SIGNATURES) {
#warning sanity checks
						sigs.firmware_signature = static_cast<uint16_t>(pch[1] | (pch[2] << 8));
						sigs.flash_signature = static_cast<uint16_t>(pch[3] | (pch[4] << 8));
						send_connection.disconnect();
						receive_connection.disconnect();
						Ptr pthis(this);
						self_ref.reset();
						signal_done.emit(pthis);
					} else {
#warning TODO something sensible
					}
				}
			}
	};

	void discard_result(AsyncOperation<void>::Ptr op) {
		op->result();
	}
}

AsyncOperation<void>::Ptr XBeeRobot::firmware_spi_chip_erase() {
	return FirmwareSPIChipEraseOperation::create(dongle, index);
}

AsyncOperation<void>::Ptr XBeeRobot::firmware_spi_fill_page_buffer(unsigned int offset, const void *data, std::size_t length) {
	return FirmwareSPIFillPageBufferOperation::create(dongle, index, offset, data, length);
}

AsyncOperation<void>::Ptr XBeeRobot::firmware_spi_page_program(unsigned int page, uint16_t crc) {
	return FirmwareSPIPageProgramOperation::create(dongle, index, page, crc);
}

AsyncOperation<uint16_t>::Ptr XBeeRobot::firmware_spi_block_crc(unsigned int address, std::size_t length) {
	return FirmwareSPIBlockCRCOperation::create(dongle, index, address, length);
}

AsyncOperation<XBeeRobot::OperationalParameters>::Ptr XBeeRobot::firmware_read_operational_parameters() {
	return FirmwareReadOperationalParametersOperation::create(dongle, index);
}

AsyncOperation<void>::Ptr XBeeRobot::firmware_set_operational_parameters(const OperationalParameters &params) {
	return FirmwareSetOperationalParametersOperation::create(dongle, index, params);
}

AsyncOperation<void>::Ptr XBeeRobot::firmware_commit_operational_parameters() {
	return FirmwareCommitOperationalParametersOperation::create(dongle, index);
}

AsyncOperation<void>::Ptr XBeeRobot::firmware_reboot() {
	return FirmwareRebootOperation::create(dongle, index);
}

AsyncOperation<XBeeRobot::BuildSignatures>::Ptr XBeeRobot::firmware_read_build_signatures() {
	return FirmwareReadBuildSignaturesOperation::create(dongle, index);
}

void XBeeRobot::drive(const int(&wheels)[4]) {
	BitArray<80> buffer = drive_block;
	buffer.set(0, true);
	for (std::size_t i = 0; i < 4; ++i) {
		assert(-1023 <= wheels[i] && wheels[i] <= 1023);
		uint16_t u16 = static_cast<int16_t>(wheels[i]);
		for (std::size_t j = 0; j < 11; ++j) {
			buffer.set(8 + i * 11 + j, !!(u16 & (1 << j)));
		}
	}

	if (buffer != drive_block) {
		drive_block = buffer;
		flush_drive();
	}
}

void XBeeRobot::drive(int w1, int w2, int w3, int w4) {
	const int wheels[] = { w1, w2, w3, w4 };
	drive(wheels);
}

void XBeeRobot::drive_scram() {
	if (drive_block.get(0)) {
		drive_block.set(0, false);
		flush_drive();
	}
}

void XBeeRobot::dribble(bool active) {
	if (drive_block.get(2) != active) {
		drive_block.set(2, active);
		flush_drive();
	}
}

void XBeeRobot::enable_chicker(bool active) {
	if (drive_block.get(1) != active) {
		drive_block.set(1, active);
		flush_drive();
	}
}

void XBeeRobot::autokick(unsigned int pulse_width1, unsigned int pulse_width2, int offset) {
	assert(pulse_width1 <= 4064);
	assert(pulse_width2 <= 4064);
	assert(-4094 <= offset && offset <= 4094);

	/* Translate from actual widths and offset to "total-widths" and "slice width" as used by firmware/VHDL. */
	unsigned int slice_width;
	bool ignore_slice1, ignore_slice2;
#warning comment is shit at explaining what actually happens here
	if (offset < 0) {
		slice_width = -offset;
		pulse_width2 += slice_width;
		assert(pulse_width2 <= 4064);
		ignore_slice1 = true;
		ignore_slice2 = false;
	} else if (offset > 0) {
		slice_width = offset;
		pulse_width1 += slice_width;
		assert(pulse_width1 <= 4064);
		ignore_slice1 = false;
		ignore_slice2 = true;
	} else {
		slice_width = 0;
		ignore_slice1 = true;
		ignore_slice2 = true;
	}

	BitArray<80> buffer = drive_block;
	buffer.set(5, pulse_width1 || pulse_width2);
	for (std::size_t i = 0; i < 7; ++i) {
		buffer.set(58 + i, !!(pulse_width1 & (1 << i)));
	}
	for (std::size_t i = 0; i < 7; ++i) {
		buffer.set(65 + i, !!(pulse_width2 & (1 << i)));
	}
	for (std::size_t i = 0; i < 7; ++i) {
		buffer.set(72 + i, !!(slice_width & (1 << i)));
	}
	buffer.set(3, ignore_slice1);
	buffer.set(4, ignore_slice2);

	if (buffer != drive_block) {
		drive_block = buffer;
		flush_drive();
	}
}

void XBeeRobot::kick(unsigned int pulse_width1, unsigned int pulse_width2, int offset) {
	assert(pulse_width1 <= 4064);
	assert(pulse_width2 <= 4064);
	assert(-4094 <= offset && offset <= 4094);

	/* Translate from actual widths and offset to "total-widths" and "slice width" as used by firmware/VHDL. */
	unsigned int slice_width;
	bool ignore_slice1, ignore_slice2;
#warning comment is shit at explaining what actually happens here
	if (offset < 0) {
		slice_width = -offset;
		pulse_width2 += slice_width;
		assert(pulse_width2 <= 4064);
		ignore_slice1 = true;
		ignore_slice2 = false;
	} else if (offset > 0) {
		slice_width = offset;
		pulse_width1 += slice_width;
		assert(pulse_width1 <= 4064);
		ignore_slice1 = false;
		ignore_slice2 = true;
	} else {
		slice_width = 0;
		ignore_slice1 = true;
		ignore_slice2 = true;
	}

	uint8_t buffer[5];
	buffer[0] = static_cast<uint8_t>(index << 4) | XBeeDongle::PIPE_KICK;
	buffer[1] = 0x00;
	buffer[2] = static_cast<uint8_t>((ignore_slice1 ? 0x80 : 0x00) | (pulse_width1 / 32));
	buffer[3] = static_cast<uint8_t>((ignore_slice2 ? 0x80 : 0x00) | (pulse_width2 / 32));
	buffer[4] = static_cast<uint8_t>(slice_width / 32);

	LibUSBInterruptOutTransfer::Ptr transfer = LibUSBInterruptOutTransfer::create(dongle.device, XBeeDongle::EP_MESSAGE, buffer, sizeof(buffer), 0, 5);
	transfer->signal_done.connect(&discard_result);
	transfer->submit();
}

void XBeeRobot::test_mode(unsigned int mode) {
	assert((mode & 0x77) == mode);
	BitArray<80> buffer = drive_block;
	buffer.set(52, !!(mode & 0x10));
	buffer.set(53, !!(mode & 0x20));
	buffer.set(54, !!(mode & 0x40));
	buffer.set(55, !!(mode & 0x01));
	buffer.set(56, !!(mode & 0x02));
	buffer.set(57, !!(mode & 0x04));
	if (buffer != drive_block) {
		drive_block = buffer;
		flush_drive();
	}
}

XBeeRobot::Ptr XBeeRobot::create(XBeeDongle &dongle, unsigned int index) {
	Ptr p(new XBeeRobot(dongle, index));
	return p;
}

XBeeRobot::XBeeRobot(XBeeDongle &dongle, unsigned int index) : alive(false), has_feedback(false), ball_in_beam(false), ball_on_dribbler(false), capacitor_charged(false), battery_voltage(0), capacitor_voltage(0), dribbler_temperature(0), break_beam_reading(0), dongle(dongle), index(index) {
}

XBeeRobot::~XBeeRobot() {
}

void XBeeRobot::flush_drive() {
	dongle.dirty_drive(index);
}

void XBeeRobot::on_feedback(const uint8_t *data, std::size_t length) {
	assert(length == 9);
	has_feedback = !!(data[0] & 0x01);
	ball_in_beam = !!(data[0] & 0x02);
	ball_on_dribbler = !!(data[0] & 0x04);
	capacitor_charged = !!(data[0] & 0x08);
	uint16_t ui16 = static_cast<uint16_t>(data[1] | (data[2] << 8));
	battery_voltage = (ui16 + 0.5) / 1024.0 * 3.3 / 330.0 * (1500.0 + 330.0);
	ui16 = static_cast<uint16_t>(data[3] | (data[4] << 8));
	capacitor_voltage = ui16 / 4096.0 * 3.3 / 2200.0 * (220000.0 + 2200.0);
	ui16 = static_cast<uint16_t>(data[5] | (data[6] << 8));
	dribbler_temperature = ui16 * 0.5735 - 205.9815;
	ui16 = static_cast<uint16_t>(data[7] | (data[8] << 8));
	break_beam_reading = ui16;
#warning deal with faults
}

