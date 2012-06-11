#include "xbee/dongle.h"
#include "xbee/robot.h"
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

namespace std {
	template<> class hash<XBeeDongle::CommonFault> {
		public:
			std::size_t operator()(XBeeDongle::CommonFault f) const {
				return h(f);
			}

		private:
			std::hash<unsigned int> h;
	};

	template<> class hash<XBeeDongle::DongleFault> {
		public:
			std::size_t operator()(XBeeDongle::DongleFault f) const {
				return h(f);
			}

		private:
			std::hash<unsigned int> h;
	};

	template<> class hash<XBeeDongle::RobotFault> {
		public:
			std::size_t operator()(XBeeDongle::RobotFault f) const {
				return h(f);
			}

		private:
			std::hash<unsigned int> h;
	};
}

namespace {
	enum TBotsControlRequest {
		TBOTS_CONTROL_REQUEST_GET_XBEE_FW_VERSION = 0x00,
		TBOTS_CONTROL_REQUEST_GET_XBEE_CHANNELS = 0x01,
		TBOTS_CONTROL_REQUEST_SET_XBEE_CHANNELS = 0x02,
		TBOTS_CONTROL_REQUEST_ENABLE_RADIOS = 0x03,
		TBOTS_CONTROL_REQUEST_RESEND_DONGLE_STATUS = 0x04,
	};

	template<typename T> struct FaultMessageInfo {
		T code;
		const char *message;
		Annunciator::Message::TriggerMode mode;
	};

	const FaultMessageInfo<XBeeDongle::CommonFault> COMMON_FAULT_MESSAGE_INFOS[] = {
		{ XBeeDongle::FAULT_XBEE0_FERR, "XBee 0 framing error", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_XBEE1_FERR, "XBee 1 framing error", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_XBEE0_OERR_HW, "XBee 0 hardware overrun error", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_XBEE1_OERR_HW, "XBee 1 hardware overrun error", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_XBEE0_OERR_SW, "XBee 0 software overrun error", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_XBEE1_OERR_SW, "XBee 1 software overrun error", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_XBEE0_CHECKSUM_FAILED, "XBee 0 checksum failed", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_XBEE1_CHECKSUM_FAILED, "XBee 1 checksum failed", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_XBEE0_NONZERO_LENGTH_MSB, "XBee 0 nonzero length MSB", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_XBEE1_NONZERO_LENGTH_MSB, "XBee 1 nonzero length MSB", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_XBEE0_INVALID_LENGTH_LSB, "XBee 0 invalid length LSB", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_XBEE1_INVALID_LENGTH_LSB, "XBee 1 invalid length LSB", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_XBEE0_TIMEOUT, "XBee 0 timeout", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_XBEE1_TIMEOUT, "XBee 1 timeout", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_XBEE0_AT_RESPONSE_WRONG_COMMAND, "XBee 0 AT response wrong command", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_XBEE1_AT_RESPONSE_WRONG_COMMAND, "XBee 1 AT response wrong command", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_XBEE0_AT_RESPONSE_FAILED_UNKNOWN_REASON, "XBee 0 AT response failed: unknown reason", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_XBEE1_AT_RESPONSE_FAILED_UNKNOWN_REASON, "XBee 1 AT response failed: unknown reason", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_XBEE0_AT_RESPONSE_FAILED_INVALID_COMMAND, "XBee 0 AT response failed: invalid command", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_XBEE1_AT_RESPONSE_FAILED_INVALID_COMMAND, "XBee 1 AT response failed: invalid command", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_XBEE0_AT_RESPONSE_FAILED_INVALID_PARAMETER, "XBee 0 AT response failed: invalid parameter", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_XBEE1_AT_RESPONSE_FAILED_INVALID_PARAMETER, "XBee 1 AT response failed: invalid parameter", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_XBEE0_RESET_FAILED, "XBee 0 failed to reset", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_XBEE1_RESET_FAILED, "XBee 1 failed to reset", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_XBEE0_ENABLE_RTS_FAILED, "XBee 0 failed to enable RTS", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_XBEE1_ENABLE_RTS_FAILED, "XBee 1 failed to enable RTS", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_XBEE0_GET_FW_VERSION_FAILED, "XBee 0 failed to get firmware version", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_XBEE1_GET_FW_VERSION_FAILED, "XBee 1 failed to get firmware version", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_XBEE0_SET_NODE_ID_FAILED, "XBee 0 failed to set text node ID", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_XBEE1_SET_NODE_ID_FAILED, "XBee 1 failed to set text node ID", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_XBEE0_SET_CHANNEL_FAILED, "XBee 0 failed to set radio channel", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_XBEE1_SET_CHANNEL_FAILED, "XBee 1 failed to set radio channel", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_XBEE0_SET_PAN_ID_FAILED, "XBee 0 failed to set PAN ID", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_XBEE1_SET_PAN_ID_FAILED, "XBee 1 failed to set PAN ID", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_XBEE0_SET_ADDRESS_FAILED, "XBee 0 failed to set 16-bit address", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_XBEE1_SET_ADDRESS_FAILED, "XBee 1 failed to set 16-bit address", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_OUT_MICROPACKET_OVERFLOW, "Outbound micropacket overflow", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_OUT_MICROPACKET_NOPIPE, "Outbound micropacket to invalid pipe", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_OUT_MICROPACKET_BAD_LENGTH, "Outbound micropacket invalid length", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_DEBUG_OVERFLOW, "Debug endpoint overflow", Annunciator::Message::TriggerMode::EDGE },
	};

	const FaultMessageInfo<XBeeDongle::DongleFault> DONGLE_FAULT_MESSAGE_INFOS[] = {
		{ XBeeDongle::FAULT_ERROR_QUEUE_OVERFLOW, "Local error queue overflow", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT0, "Failed to send to robot 0", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT1, "Failed to send to robot 1", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT2, "Failed to send to robot 2", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT3, "Failed to send to robot 3", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT4, "Failed to send to robot 4", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT5, "Failed to send to robot 5", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT6, "Failed to send to robot 6", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT7, "Failed to send to robot 7", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT8, "Failed to send to robot 8", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT9, "Failed to send to robot 9", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT10, "Failed to send to robot 10", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT11, "Failed to send to robot 11", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT12, "Failed to send to robot 12", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT13, "Failed to send to robot 13", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT14, "Failed to send to robot 14", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT15, "Failed to send to robot 15", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT0, "Inbound micropacket overflow from robot 0", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT1, "Inbound micropacket overflow from robot 1", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT2, "Inbound micropacket overflow from robot 2", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT3, "Inbound micropacket overflow from robot 3", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT4, "Inbound micropacket overflow from robot 4", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT5, "Inbound micropacket overflow from robot 5", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT6, "Inbound micropacket overflow from robot 6", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT7, "Inbound micropacket overflow from robot 7", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT8, "Inbound micropacket overflow from robot 8", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT9, "Inbound micropacket overflow from robot 9", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT10, "Inbound micropacket overflow from robot 10", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT11, "Inbound micropacket overflow from robot 11", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT12, "Inbound micropacket overflow from robot 12", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT13, "Inbound micropacket overflow from robot 13", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT14, "Inbound micropacket overflow from robot 14", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT15, "Inbound micropacket overflow from robot 15", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT0, "Inbound micropacket to invalid pipe from robot 0", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT1, "Inbound micropacket to invalid pipe from robot 1", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT2, "Inbound micropacket to invalid pipe from robot 2", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT3, "Inbound micropacket to invalid pipe from robot 3", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT4, "Inbound micropacket to invalid pipe from robot 4", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT5, "Inbound micropacket to invalid pipe from robot 5", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT6, "Inbound micropacket to invalid pipe from robot 6", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT7, "Inbound micropacket to invalid pipe from robot 7", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT8, "Inbound micropacket to invalid pipe from robot 8", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT9, "Inbound micropacket to invalid pipe from robot 9", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT10, "Inbound micropacket to invalid pipe from robot 10", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT11, "Inbound micropacket to invalid pipe from robot 11", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT12, "Inbound micropacket to invalid pipe from robot 12", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT13, "Inbound micropacket to invalid pipe from robot 13", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT14, "Inbound micropacket to invalid pipe from robot 14", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT15, "Inbound micropacket to invalid pipe from robot 15", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT0, "Inbound micropacket invalid length from robot 0", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT1, "Inbound micropacket invalid length from robot 1", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT2, "Inbound micropacket invalid length from robot 2", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT3, "Inbound micropacket invalid length from robot 3", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT4, "Inbound micropacket invalid length from robot 4", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT5, "Inbound micropacket invalid length from robot 5", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT6, "Inbound micropacket invalid length from robot 6", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT7, "Inbound micropacket invalid length from robot 7", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT8, "Inbound micropacket invalid length from robot 8", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT9, "Inbound micropacket invalid length from robot 9", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT10, "Inbound micropacket invalid length from robot 10", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT11, "Inbound micropacket invalid length from robot 11", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT12, "Inbound micropacket invalid length from robot 12", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT13, "Inbound micropacket invalid length from robot 13", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT14, "Inbound micropacket invalid length from robot 14", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT15, "Inbound micropacket invalid length from robot 15", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_BTSEF, "Bit stuff error", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_BTOEF, "Bus turnaround timeout", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_DFN8EF, "Non-integral data field byte count", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_CRC16EF, "CRC16 error", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_CRC5EF, "CRC5 error", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_PIDEF, "PID check error", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_STALL, "Stall handshake returned", Annunciator::Message::TriggerMode::EDGE },
	};

	const FaultMessageInfo<XBeeDongle::RobotFault> ROBOT_FAULT_MESSAGE_INFOS[] = {
		{ XBeeDongle::FAULT_CAPACITOR_CHARGE_TIMEOUT, "Capacitor charge timeout", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_CHICKER_COMM_ERROR, "Chicker communication error", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_CHICKER_NOT_PRESENT, "Chicker not present", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_FPGA_NO_BITSTREAM, "FPGA no bitstream", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_FPGA_INVALID_BITSTREAM, "FPGA invalid bitstream", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_FPGA_DCM_LOCK_FAILED, "FPGA DCM lock failed", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_FPGA_ONLINE_CRC_FAILED, "FPGA online fabric CRC failed", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_FPGA_COMM_ERROR, "FPGA communication error", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_OSCILLATOR_FAILED, "Oscillator failed", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_MOTOR1_HALL_000, "Motor 1 Hall sensor invalid code 000", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_MOTOR2_HALL_000, "Motor 2 Hall sensor invalid code 000", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_MOTOR3_HALL_000, "Motor 3 Hall sensor invalid code 000", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_MOTOR4_HALL_000, "Motor 4 Hall sensor invalid code 000", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_MOTORD_HALL_000, "Motor D Hall sensor invalid code 000", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_MOTOR1_HALL_111, "Motor 1 Hall sensor invalid code 111", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_MOTOR2_HALL_111, "Motor 2 Hall sensor invalid code 111", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_MOTOR3_HALL_111, "Motor 3 Hall sensor invalid code 111", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_MOTOR4_HALL_111, "Motor 4 Hall sensor invalid code 111", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_MOTORD_HALL_111, "Motor D Hall sensor invalid code 111", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_MOTOR1_HALL_STUCK, "Motor 1 Hall sensor stuck w.r.t. optical encoder", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_MOTOR2_HALL_STUCK, "Motor 2 Hall sensor stuck w.r.t. optical encoder", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_MOTOR3_HALL_STUCK, "Motor 3 Hall sensor stuck w.r.t. optical encoder", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_MOTOR4_HALL_STUCK, "Motor 4 Hall sensor stuck w.r.t. optical encoder", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_MOTOR1_ENCODER_STUCK, "Motor 1 optical encoder stuck w.r.t. Hall sensor", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_MOTOR2_ENCODER_STUCK, "Motor 2 optical encoder stuck w.r.t. Hall sensor", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_MOTOR3_ENCODER_STUCK, "Motor 3 optical encoder stuck w.r.t. Hall sensor", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_MOTOR4_ENCODER_STUCK, "Motor 4 optical encoder stuck w.r.t. Hall sensor", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_DRIBBLER_OVERHEAT, "Dribbler overheating", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_DRIBBLER_THERMISTOR_FAILED, "Dribbler thermistor communication error", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_FLASH_COMM_ERROR, "Flash communication error", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_FLASH_PARAMS_CORRUPT, "Flash operational parameters block corrupt", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_NO_PARAMS, "Flash operational parameters block absent", Annunciator::Message::TriggerMode::LEVEL },
		{ XBeeDongle::FAULT_IN_PACKET_OVERFLOW, "Inbound packet overflow", Annunciator::Message::TriggerMode::EDGE },
		{ XBeeDongle::FAULT_FIRMWARE_BAD_REQUEST, "Bad firmware request", Annunciator::Message::TriggerMode::EDGE },
	};

	template<typename T> class FaultMessage : public Annunciator::Message {
		public:
			static typename std::unordered_map<T, FaultMessage<T> *> INSTANCES;

			FaultMessage(const FaultMessageInfo<T> *array_base, unsigned int index) : Annunciator::Message(array_base[index].message, array_base[index].mode) {
				instances_rw()[array_base[index].code] = this;
			}

			static const std::unordered_map<T, FaultMessage<T> *> &instances() {
				return instances_rw();
			}

		private:
			static std::unordered_map<T, FaultMessage<T> *> &instances_rw() {
				static std::unordered_map<T, FaultMessage<T> *> m;
				return m;
			}
	};

	class CommonFaultMessage : public FaultMessage<XBeeDongle::CommonFault> {
		public:
			CommonFaultMessage() : FaultMessage<XBeeDongle::CommonFault>(COMMON_FAULT_MESSAGE_INFOS, static_cast<unsigned int>(this - INSTANCE_ARRAY)) {
			}

		private:
			static CommonFaultMessage INSTANCE_ARRAY[G_N_ELEMENTS(COMMON_FAULT_MESSAGE_INFOS)];
	};

	CommonFaultMessage CommonFaultMessage::INSTANCE_ARRAY[G_N_ELEMENTS(COMMON_FAULT_MESSAGE_INFOS)];

	class DongleFaultMessage : public FaultMessage<XBeeDongle::DongleFault> {
		public:
			DongleFaultMessage() : FaultMessage<XBeeDongle::DongleFault>(DONGLE_FAULT_MESSAGE_INFOS, static_cast<unsigned int>(this - INSTANCE_ARRAY)) {
			}

		private:
			static DongleFaultMessage INSTANCE_ARRAY[G_N_ELEMENTS(DONGLE_FAULT_MESSAGE_INFOS)];
	};

	DongleFaultMessage DongleFaultMessage::INSTANCE_ARRAY[G_N_ELEMENTS(DONGLE_FAULT_MESSAGE_INFOS)];

	class RobotFaultMessage : public FaultMessage<XBeeDongle::RobotFault> {
		public:
			RobotFaultMessage() : FaultMessage<XBeeDongle::RobotFault>(ROBOT_FAULT_MESSAGE_INFOS, static_cast<unsigned int>(this - INSTANCE_ARRAY)) {
			}

		private:
			static RobotFaultMessage INSTANCE_ARRAY[G_N_ELEMENTS(ROBOT_FAULT_MESSAGE_INFOS)];
	};

	RobotFaultMessage RobotFaultMessage::INSTANCE_ARRAY[G_N_ELEMENTS(ROBOT_FAULT_MESSAGE_INFOS)];

	Annunciator::Message unknown_local_error_message("Dongle local error queue contained unknown error code", Annunciator::Message::TriggerMode::EDGE);
	Annunciator::Message in_micropacket_overflow_message("Inbound micropacket overflow from dongle", Annunciator::Message::TriggerMode::EDGE);

	XBeeDongle::EStopState decode_estop_state(uint8_t st) {
		XBeeDongle::EStopState est = static_cast<XBeeDongle::EStopState>(st);
		switch (est) {
			case XBeeDongle::EStopState::UNINITIALIZED:
			case XBeeDongle::EStopState::DISCONNECTED:
			case XBeeDongle::EStopState::STOP:
			case XBeeDongle::EStopState::RUN:
				return est;
		}
		throw std::runtime_error("Dongle status illegal emergency stop state");
	}

	XBeeDongle::XBeesState decode_xbees_state(uint8_t st) {
		XBeeDongle::XBeesState est = static_cast<XBeeDongle::XBeesState>(st);
		switch (est) {
			case XBeeDongle::XBeesState::PREINIT:
			case XBeeDongle::XBeesState::INIT0:
			case XBeeDongle::XBeesState::INIT1:
			case XBeeDongle::XBeesState::RUNNING:
			case XBeeDongle::XBeesState::FAIL0:
			case XBeeDongle::XBeesState::FAIL1:
				return est;
		}
		throw std::runtime_error("Dongle status illegal XBees state");
	}
}

XBeeDongle::SendMessageOperation::SendMessageOperation(XBeeDongle &dongle, const void *data, std::size_t length) : transfer(dongle.device, EP_MESSAGE, data, length, 0) {
	transfer.signal_done.connect(signal_done.make_slot());
	transfer.submit();
}

void XBeeDongle::SendMessageOperation::result() const {
	transfer.result();
}

XBeeDongle::XBeeDongle(bool force_reinit) : estop_state(EStopState::UNINITIALIZED), xbees_state(XBeesState::PREINIT), context(), device(context, 0x04D8, 0x7839), local_error_queue_transfer(device, EP_LOCAL_ERROR_QUEUE, 64, false, 0), debug_transfer(device, EP_DEBUG, 4096, false, 0), dongle_status_transfer(device, EP_DONGLE_STATUS, 4, true, 0), state_transport_in_transfer(device, EP_STATE_TRANSPORT, 64, false, 0), interrupt_in_transfer(device, EP_MESSAGE, 64, false, 0), stamp_transfer(device, EP_STATE_TRANSPORT, 0, 0, 0), dirty_drive_mask(0), enabled(false) {
	for (unsigned int i = 0; i < 16; ++i) {
		std::unique_ptr<XBeeRobot> p(new XBeeRobot(*this, i));
		robots.push_back(std::move(p));
	}
	if (force_reinit) {
		device.set_configuration(0);
		sleep(1);
	}
	if (device.get_configuration() != 1) {
		device.set_configuration(1);
		sleep(1);
	}
	device.claim_interface(0);
	device.claim_interface(1);

	local_error_queue_transfer.signal_done.connect(sigc::mem_fun(this, &XBeeDongle::on_local_error_queue));
	local_error_queue_transfer.submit();

	debug_transfer.signal_done.connect(sigc::mem_fun(this, &XBeeDongle::on_debug));
	debug_transfer.submit();
}

XBeeDongle::~XBeeDongle() = default;

void XBeeDongle::enable() {
	assert(!enabled);
	device.control_no_data(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE, TBOTS_CONTROL_REQUEST_ENABLE_RADIOS, 0x0000, 0x0000, 0);
	device.control_no_data(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE, TBOTS_CONTROL_REQUEST_RESEND_DONGLE_STATUS, 0x0000, 0x0000, 0);
	do {
		uint8_t status[4];
		if (device.interrupt_in(EP_DONGLE_STATUS, status, sizeof(status), 0) != sizeof(status)) {
			throw std::runtime_error("Dongle status block of wrong length received");
		}
		parse_dongle_status(status);
	} while (xbees_state != XBeesState::RUNNING);
	enabled = true;

	dongle_status_transfer.signal_done.connect(sigc::mem_fun(this, &XBeeDongle::on_dongle_status));
	dongle_status_transfer.submit();

	state_transport_in_transfer.signal_done.connect(sigc::mem_fun(this, &XBeeDongle::on_state_transport_in));
	state_transport_in_transfer.submit();

	interrupt_in_transfer.signal_done.connect(sigc::mem_fun(this, &XBeeDongle::on_interrupt_in));
	interrupt_in_transfer.submit();

	stamp_connection = Glib::signal_timeout().connect(sigc::bind_return(sigc::mem_fun(this, &XBeeDongle::on_stamp), true), 300);
}

std::pair<unsigned int, unsigned int> XBeeDongle::get_channels() {
	uint8_t buffer[2];
	std::size_t sz;
	if ((sz = device.control_in(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE, TBOTS_CONTROL_REQUEST_GET_XBEE_CHANNELS, 0x0000, 0x0000, buffer, sizeof(buffer), 0)) != sizeof(buffer)) {
		throw std::runtime_error(Glib::locale_from_utf8(Glib::ustring::compose("GET_XBEE_CHANNELS returned %1 bytes, expected %2", sz, sizeof(buffer))));
	}
	return std::make_pair(buffer[0], buffer[1]);
}

void XBeeDongle::set_channels(unsigned int channel0, unsigned int channel1) {
	assert(!enabled);
	assert(0x0B <= channel0 && channel0 <= 0x1A);
	assert(0x0B <= channel1 && channel1 <= 0x1A);
	device.control_no_data(LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE, TBOTS_CONTROL_REQUEST_SET_XBEE_CHANNELS, static_cast<uint16_t>(channel0 | (channel1 << 8)), 0x0000, 0);
}

void XBeeDongle::on_dongle_status(AsyncOperation<void> &) {
	dongle_status_transfer.result();
	parse_dongle_status(dongle_status_transfer.data());
	dongle_status_transfer.submit();
}

void XBeeDongle::parse_dongle_status(const uint8_t *data) {
	estop_state = decode_estop_state(data[0]);
	xbees_state = decode_xbees_state(data[1]);
	if (xbees_state == XBeesState::FAIL0) {
		throw std::runtime_error("XBee 0 failed");
	} else if (xbees_state == XBeesState::FAIL1) {
		throw std::runtime_error("XBee 1 failed");
	}
	uint16_t mask = static_cast<uint16_t>(data[2] | (data[3] << 8));
	for (unsigned int i = 0; i < robots.size(); ++i) {
		robot(i).alive = !!(mask & (1 << i));
	}
	signal_dongle_status_updated.emit();
}

void XBeeDongle::on_local_error_queue(AsyncOperation<void> &) {
	local_error_queue_transfer.result();
	for (std::size_t i = 0; i < local_error_queue_transfer.size(); ++i) {
		uint8_t code = local_error_queue_transfer.data()[i];
		Annunciator::Message *msg;
		if (CommonFaultMessage::instances().count(static_cast<CommonFault>(code))) {
			msg = CommonFaultMessage::instances().find(static_cast<CommonFault>(code))->second;
		} else if (DongleFaultMessage::instances().count(static_cast<DongleFault>(code))) {
			msg = DongleFaultMessage::instances().find(static_cast<DongleFault>(code))->second;
		} else {
			msg = &unknown_local_error_message;
		}
		msg->fire();
	}
	local_error_queue_transfer.submit();
}

void XBeeDongle::on_state_transport_in(AsyncOperation<void> &) {
	state_transport_in_transfer.result();
	for (std::size_t i = 0; i < state_transport_in_transfer.size(); i += state_transport_in_transfer.data()[i]) {
		assert(i + state_transport_in_transfer.data()[i] <= state_transport_in_transfer.size());
		unsigned int index = state_transport_in_transfer.data()[i + 1] >> 4;
		assert(index <= 15);
		unsigned int pipe = state_transport_in_transfer.data()[i + 1] & 0x0F;
		assert(pipe == PIPE_FEEDBACK);
		robot(index).on_feedback(state_transport_in_transfer.data() + i + 2, state_transport_in_transfer.data()[i] - 2);
	}
	state_transport_in_transfer.submit();
}

void XBeeDongle::on_interrupt_in(AsyncOperation<void> &) {
	interrupt_in_transfer.result();
	if (interrupt_in_transfer.size()) {
		unsigned int robot = interrupt_in_transfer.data()[0] >> 4;
		Pipe pipe = static_cast<Pipe>(interrupt_in_transfer.data()[0] & 0x0F);
		signal_message_received.emit(robot, pipe, interrupt_in_transfer.data() + 1, interrupt_in_transfer.size() - 1);
		if (pipe == Pipe::PIPE_EXPERIMENT_DATA) {
			this->robot(robot).signal_experiment_data.emit(interrupt_in_transfer.data() + 1, interrupt_in_transfer.size() - 1);
		} else if (pipe == Pipe::PIPE_AUTOKICK_INDICATOR) {
			this->robot(robot).signal_autokick_fired.emit();
		}
	}
	interrupt_in_transfer.submit();
}

void XBeeDongle::on_debug(AsyncOperation<void> &) {
	try {
		debug_transfer.result();
		if (debug_transfer.size()) {
			std::string str(reinterpret_cast<const char *>(debug_transfer.data()), debug_transfer.size());
			LOG_INFO(Glib::locale_to_utf8(str));
		}
		debug_transfer.submit();
	} catch (const USB::TransferError &err) {
		LOG_ERROR(Glib::ustring::compose("Ignoring %1", err.what()));
	}
}

void XBeeDongle::on_stamp() {
	stamp_transfer.submit();
}

void XBeeDongle::dirty_drive(unsigned int index) {
	assert(index <= 15);
	dirty_drive_mask |= 1 << index;
	if (!flush_drive_connection) {
		flush_drive_connection = Glib::signal_idle().connect(sigc::bind_return(sigc::mem_fun(this, &XBeeDongle::flush_drive), false));
	}
}

void XBeeDongle::submit_drive_transfer(const void *buffer, std::size_t length) {
	std::size_t index = std::find(drive_transfers.begin(), drive_transfers.end(), std::unique_ptr<USB::InterruptOutTransfer>()) - drive_transfers.begin();
	if (index == drive_transfers.size()) {
		drive_transfers.push_back(std::unique_ptr<USB::InterruptOutTransfer>());
	}
	drive_transfers[index].reset(new USB::InterruptOutTransfer(device, EP_STATE_TRANSPORT, buffer, length, 0));
	drive_transfers[index]->signal_done.connect(sigc::bind(sigc::mem_fun(this, &XBeeDongle::check_drive_transfer_result), index));
	drive_transfers[index]->submit();
}

void XBeeDongle::flush_drive() {
	static const std::size_t packet_size = XBeePackets::Drive::BUFFER_SIZE + 2;
	uint8_t buffer[64 / packet_size][packet_size];
	std::size_t wptr = 0;

	for (unsigned int i = 0; i <= 15; ++i) {
		if (dirty_drive_mask & (1 << i)) {
			dirty_drive_mask &= ~(1 << i);
			buffer[wptr][0] = static_cast<uint8_t>(sizeof(buffer[0]));
			buffer[wptr][1] = static_cast<uint8_t>(i << 4) | PIPE_DRIVE;
			robot(i).drive_block.encode(&buffer[wptr][2]);
			++wptr;

			if (wptr == sizeof(buffer) / sizeof(*buffer)) {
				submit_drive_transfer(buffer, wptr * packet_size);
				wptr = 0;
			}
		}
	}

	if (wptr) {
		submit_drive_transfer(buffer, wptr * packet_size);
		wptr = 0;
	}

	stamp_connection.disconnect();
	stamp_connection = Glib::signal_timeout().connect(sigc::bind_return(sigc::mem_fun(this, &XBeeDongle::on_stamp), true), 300);
}

void XBeeDongle::check_drive_transfer_result(AsyncOperation<void> &, std::size_t index) {
	drive_transfers[index]->result();
	drive_transfers[index].reset();
}

