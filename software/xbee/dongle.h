#ifndef XBEE_DONGLE_H
#define XBEE_DONGLE_H

/**
 * \file
 *
 * \brief Provides access to an XBee dongle.
 */

#include "util/async_operation.h"
#include "util/noncopyable.h"
#include "util/property.h"
#include "xbee/libusb.h"
#include "xbee/robot.h"
#include <cassert>
#include <cstddef>

/**
 * \brief The dongle.
 */
class XBeeDongle : public NonCopyable {
	public:
		/**
		 * \brief The possible states the emergency stop switch can be in.
		 */
		enum class EStopState {
			/**
			 * \brief The switch has not been sampled yet.
			 */
			UNINITIALIZED,

			/**
			 * \brief The switch is not plugged in or is not working properly.
			 */
			DISCONNECTED,

			/**
			 * \brief The switch is in the stop position.
			 */
			STOP,

			/**
			 * \brief The switch is in the run position.
			 */
			RUN,
		};

		/**
		 * \brief The possible states the XBees can be in.
		 */
		enum class XBeesState {
			/**
			 * \brief Stage 1 initialization has not yet started.
			 */
			PREINIT,

			/**
			 * \brief XBee 0 is initialized.
			 */
			INIT0,

			/**
			 * \brief XBee 1 is initialized.
			 */
			INIT1,

			/**
			 * \brief The XBees have completed all initialization and are communicating normally.
			 */
			RUNNING,

			/**
			 * \brief XBee 0 has failed.
			 */
			FAIL0,

			/**
			 * \brief XBee 1 has failed.
			 */
			FAIL1,
		};

		/**
		 * \brief The pipes between the host and a robot.
		 */
		enum Pipe {
			/**
			 * \brief A state transport pipe that carries drive information to the robot.
			 */
			PIPE_DRIVE,

			/**
			 * \brief A state transport pipe that carries feedback information from the robot.
			 */
			PIPE_FEEDBACK,

			/**
			 * \brief An interrupt pipe that allows the robot to be ordered to kick or chip.
			 */
			PIPE_KICK,

			/**
			 * \brief A bulk pipe that allows firmware upgrades to be executed.
			 */
			PIPE_FIRMWARE_OUT,

			/**
			 * \brief An interrupt pipe that carries responses to firmware upgrade operation requests
			 */
			PIPE_FIRMWARE_IN,
		};

		/**
		 * \brief The fault codes that can be reported by both a dongle and a robot.
		 */
		enum CommonFault {
			FAULT_XBEE0_FERR,
			FAULT_XBEE1_FERR,
			FAULT_XBEE0_OERR_HW,
			FAULT_XBEE1_OERR_HW,
			FAULT_XBEE0_OERR_SW,
			FAULT_XBEE1_OERR_SW,
			FAULT_XBEE0_CHECKSUM_FAILED,
			FAULT_XBEE1_CHECKSUM_FAILED,
			FAULT_XBEE0_NONZERO_LENGTH_MSB,
			FAULT_XBEE1_NONZERO_LENGTH_MSB,
			FAULT_XBEE0_INVALID_LENGTH_LSB,
			FAULT_XBEE1_INVALID_LENGTH_LSB,
			FAULT_XBEE0_TIMEOUT,
			FAULT_XBEE1_TIMEOUT,
			FAULT_XBEE0_AT_RESPONSE_WRONG_COMMAND,
			FAULT_XBEE1_AT_RESPONSE_WRONG_COMMAND,
			FAULT_XBEE0_AT_RESPONSE_FAILED_UNKNOWN_REASON,
			FAULT_XBEE1_AT_RESPONSE_FAILED_UNKNOWN_REASON,
			FAULT_XBEE0_AT_RESPONSE_FAILED_INVALID_COMMAND,
			FAULT_XBEE1_AT_RESPONSE_FAILED_INVALID_COMMAND,
			FAULT_XBEE0_AT_RESPONSE_FAILED_INVALID_PARAMETER,
			FAULT_XBEE1_AT_RESPONSE_FAILED_INVALID_PARAMETER,
			FAULT_XBEE0_RESET_FAILED,
			FAULT_XBEE1_RESET_FAILED,
			FAULT_XBEE0_ENABLE_RTS_FAILED,
			FAULT_XBEE1_ENABLE_RTS_FAILED,
			FAULT_XBEE0_GET_FW_VERSION_FAILED,
			FAULT_XBEE1_GET_FW_VERSION_FAILED,
			FAULT_XBEE0_SET_NODE_ID_FAILED,
			FAULT_XBEE1_SET_NODE_ID_FAILED,
			FAULT_XBEE0_SET_CHANNEL_FAILED,
			FAULT_XBEE1_SET_CHANNEL_FAILED,
			FAULT_XBEE0_SET_PAN_ID_FAILED,
			FAULT_XBEE1_SET_PAN_ID_FAILED,
			FAULT_XBEE0_SET_ADDRESS_FAILED,
			FAULT_XBEE1_SET_ADDRESS_FAILED,
			FAULT_OUT_MICROPACKET_OVERFLOW,
			FAULT_OUT_MICROPACKET_NOPIPE,
			FAULT_OUT_MICROPACKET_BAD_LENGTH,
			FAULT_DEBUG_OVERFLOW,
			FAULT_COMMON_COUNT,
		};

		/**
		 * \brief The fault codes that can be reported only by the dongle.
		 */
		enum DongleFault {
			FAULT_ERROR_QUEUE_OVERFLOW = FAULT_COMMON_COUNT,
			FAULT_SEND_FAILED_ROBOT0,
			FAULT_SEND_FAILED_ROBOT1,
			FAULT_SEND_FAILED_ROBOT2,
			FAULT_SEND_FAILED_ROBOT3,
			FAULT_SEND_FAILED_ROBOT4,
			FAULT_SEND_FAILED_ROBOT5,
			FAULT_SEND_FAILED_ROBOT6,
			FAULT_SEND_FAILED_ROBOT7,
			FAULT_SEND_FAILED_ROBOT8,
			FAULT_SEND_FAILED_ROBOT9,
			FAULT_SEND_FAILED_ROBOT10,
			FAULT_SEND_FAILED_ROBOT11,
			FAULT_SEND_FAILED_ROBOT12,
			FAULT_SEND_FAILED_ROBOT13,
			FAULT_SEND_FAILED_ROBOT14,
			FAULT_SEND_FAILED_ROBOT15,
			FAULT_IN_MICROPACKET_OVERFLOW_ROBOT0,
			FAULT_IN_MICROPACKET_OVERFLOW_ROBOT1,
			FAULT_IN_MICROPACKET_OVERFLOW_ROBOT2,
			FAULT_IN_MICROPACKET_OVERFLOW_ROBOT3,
			FAULT_IN_MICROPACKET_OVERFLOW_ROBOT4,
			FAULT_IN_MICROPACKET_OVERFLOW_ROBOT5,
			FAULT_IN_MICROPACKET_OVERFLOW_ROBOT6,
			FAULT_IN_MICROPACKET_OVERFLOW_ROBOT7,
			FAULT_IN_MICROPACKET_OVERFLOW_ROBOT8,
			FAULT_IN_MICROPACKET_OVERFLOW_ROBOT9,
			FAULT_IN_MICROPACKET_OVERFLOW_ROBOT10,
			FAULT_IN_MICROPACKET_OVERFLOW_ROBOT11,
			FAULT_IN_MICROPACKET_OVERFLOW_ROBOT12,
			FAULT_IN_MICROPACKET_OVERFLOW_ROBOT13,
			FAULT_IN_MICROPACKET_OVERFLOW_ROBOT14,
			FAULT_IN_MICROPACKET_OVERFLOW_ROBOT15,
			FAULT_IN_MICROPACKET_NOPIPE_ROBOT0,
			FAULT_IN_MICROPACKET_NOPIPE_ROBOT1,
			FAULT_IN_MICROPACKET_NOPIPE_ROBOT2,
			FAULT_IN_MICROPACKET_NOPIPE_ROBOT3,
			FAULT_IN_MICROPACKET_NOPIPE_ROBOT4,
			FAULT_IN_MICROPACKET_NOPIPE_ROBOT5,
			FAULT_IN_MICROPACKET_NOPIPE_ROBOT6,
			FAULT_IN_MICROPACKET_NOPIPE_ROBOT7,
			FAULT_IN_MICROPACKET_NOPIPE_ROBOT8,
			FAULT_IN_MICROPACKET_NOPIPE_ROBOT9,
			FAULT_IN_MICROPACKET_NOPIPE_ROBOT10,
			FAULT_IN_MICROPACKET_NOPIPE_ROBOT11,
			FAULT_IN_MICROPACKET_NOPIPE_ROBOT12,
			FAULT_IN_MICROPACKET_NOPIPE_ROBOT13,
			FAULT_IN_MICROPACKET_NOPIPE_ROBOT14,
			FAULT_IN_MICROPACKET_NOPIPE_ROBOT15,
			FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT0,
			FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT1,
			FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT2,
			FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT3,
			FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT4,
			FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT5,
			FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT6,
			FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT7,
			FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT8,
			FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT9,
			FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT10,
			FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT11,
			FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT12,
			FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT13,
			FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT14,
			FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT15,
			FAULT_BTSEF,
			FAULT_BTOEF,
			FAULT_DFN8EF,
			FAULT_CRC16EF,
			FAULT_CRC5EF,
			FAULT_PIDEF,
			FAULT_STALL,
		};

		/**
		 * \brief The fault codes that can be reported only by a robot.
		 */
		enum RobotFault {
			FAULT_CAPACITOR_CHARGE_TIMEOUT = FAULT_COMMON_COUNT,
			FAULT_CHICKER_COMM_ERROR,
			FAULT_CHICKER_NOT_PRESENT,
			FAULT_FPGA_NO_BITSTREAM,
			FAULT_FPGA_INVALID_BITSTREAM,
			FAULT_FPGA_DCM_LOCK_FAILED,
			FAULT_FPGA_ONLINE_CRC_FAILED,
			FAULT_FPGA_COMM_ERROR,
			FAULT_OSCILLATOR_FAILED,
			FAULT_MOTOR1_HALL_000,
			FAULT_MOTOR2_HALL_000,
			FAULT_MOTOR3_HALL_000,
			FAULT_MOTOR4_HALL_000,
			FAULT_MOTORD_HALL_000,
			FAULT_MOTOR1_HALL_111,
			FAULT_MOTOR2_HALL_111,
			FAULT_MOTOR3_HALL_111,
			FAULT_MOTOR4_HALL_111,
			FAULT_MOTORD_HALL_111,
			FAULT_MOTOR1_HALL_STUCK,
			FAULT_MOTOR2_HALL_STUCK,
			FAULT_MOTOR3_HALL_STUCK,
			FAULT_MOTOR4_HALL_STUCK,
			FAULT_MOTOR1_ENCODER_STUCK,
			FAULT_MOTOR2_ENCODER_STUCK,
			FAULT_MOTOR3_ENCODER_STUCK,
			FAULT_MOTOR4_ENCODER_STUCK,
			FAULT_DRIBBLER_OVERHEAT,
			FAULT_DRIBBLER_THERMISTOR_FAILED,
			FAULT_FLASH_COMM_ERROR,
			FAULT_FLASH_PARAMS_CORRUPT,
			FAULT_NO_PARAMS,
			FAULT_IN_PACKET_OVERFLOW,
			FAULT_FIRMWARE_BAD_REQUEST,
			FAULT_ROBOT_COUNT,
		};

		/**
		 * \brief The USB endpoints.
		 */
		enum Endpoint {
			EP_DONGLE_STATUS = 1,
			EP_LOCAL_ERROR_QUEUE = 2,
			EP_STATISTICS = 3,
			EP_DEBUG = 4,
			EP_STATE_TRANSPORT = 5,
			EP_MESSAGE = 6,
		};

		/**
		 * \brief Emitted when a dongle status update is received.
		 */
		sigc::signal<void> signal_dongle_status_updated;

		/**
		 * \brief The current state of the emergency stop switch.
		 */
		Property<EStopState> estop_state;

		/**
		 * \brief The current state of the XBees.
		 */
		Property<XBeesState> xbees_state;

		/**
		 * \brief Emitted when a message is received on an interrupt pipe.
		 *
		 * \param[in] robot the robot number.
		 *
		 * \param[in] pipe the pipe on which the message was received.
		 *
		 * \param[in] data the message.
		 *
		 * \param[in] length the length of the message.
		 */
		sigc::signal<void, unsigned int, Pipe, const void *, std::size_t> signal_interrupt_message_received;

		/**
		 * \brief Constructs a new XBeeDongle.
		 */
		XBeeDongle();

		/**
		 * \brief Enables the radios.
		 */
		void enable();

		/**
		 * \brief Queues a message for transmission.
		 *
		 * \param[in] data the data to send, which must include the header.
		 *
		 * \param[in] len the length of the data, including the header.
		 */
		AsyncOperation<void>::Ptr send_message(const void *data, std::size_t len);

		/**
		 * \brief Fetches an individual robot proxy.
		 *
		 * \param[in] i the robot number.
		 *
		 * \return the robot proxy object that allows communication with the robot.
		 */
		XBeeRobot::Ptr robot(unsigned int i) {
			assert(i <= 15);
			return robots[i];
		}

	private:
		friend class XBeeRobot;

		LibUSBContext context;
		LibUSBDeviceHandle device;
		sigc::connection dongle_status_connection;
		XBeeRobot::Ptr robots[16];
		unsigned int dirty_drive_mask;
		sigc::connection flush_drive_connection;
		sigc::connection stamp_connection;
		bool enabled;

		void on_dongle_status(AsyncOperation<void>::Ptr, LibUSBInterruptInTransfer::Ptr transfer);
		void parse_dongle_status(const uint8_t *data);
		void on_local_error_queue(AsyncOperation<void>::Ptr, LibUSBInterruptInTransfer::Ptr transfer);
		void on_state_transport_in(AsyncOperation<void>::Ptr, LibUSBInterruptInTransfer::Ptr transfer);
		void on_interrupt_in(AsyncOperation<void>::Ptr, LibUSBInterruptInTransfer::Ptr transfer);
		void on_debug(AsyncOperation<void>::Ptr, LibUSBInterruptInTransfer::Ptr transfer);
		void on_stamp();
		void dirty_drive(unsigned int index);
		void flush_drive();
};

#endif

