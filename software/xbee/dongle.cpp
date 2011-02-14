#include "xbee/dongle.h"
#include "util/annunciator.h"
#include "util/dprint.h"
#include <cassert>
#include <cstring>
#include <unordered_map>

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
	void discard_result(AsyncOperation<void>::Ptr op) {
		op->result();
	}
}

namespace {
	const unsigned int STALL_LIMIT = 100;

	template<typename T> struct FaultMessageInfo {
		T code;
		const char *message;
		Annunciator::Message::TriggerMode mode;
	};

	const FaultMessageInfo<XBeeDongle::CommonFault> COMMON_FAULT_MESSAGE_INFOS[] = {
		{ XBeeDongle::FAULT_XBEE0_FERR, "XBee 0 framing error", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_XBEE1_FERR, "XBee 1 framing error", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_XBEE0_OERR_HW, "XBee 0 hardware overrun error", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_XBEE1_OERR_HW, "XBee 1 hardware overrun error", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_XBEE0_OERR_SW, "XBee 0 software overrun error", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_XBEE1_OERR_SW, "XBee 1 software overrun error", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_XBEE0_CHECKSUM_FAILED, "XBee 0 checksum failed", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_XBEE1_CHECKSUM_FAILED, "XBee 1 checksum failed", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_XBEE0_NONZERO_LENGTH_MSB, "XBee 0 nonzero length MSB", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_XBEE1_NONZERO_LENGTH_MSB, "XBee 1 nonzero length MSB", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_XBEE0_INVALID_LENGTH_LSB, "XBee 0 invalid length LSB", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_XBEE1_INVALID_LENGTH_LSB, "XBee 1 invalid length LSB", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_XBEE0_TIMEOUT, "XBee 0 timeout", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_XBEE1_TIMEOUT, "XBee 1 timeout", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_XBEE0_AT_RESPONSE_WRONG_COMMAND, "XBee 0 AT response wrong command", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_XBEE1_AT_RESPONSE_WRONG_COMMAND, "XBee 1 AT response wrong command", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_XBEE0_AT_RESPONSE_FAILED_UNKNOWN_REASON, "XBee 0 AT response failed: unknown reason", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_XBEE1_AT_RESPONSE_FAILED_UNKNOWN_REASON, "XBee 1 AT response failed: unknown reason", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_XBEE0_AT_RESPONSE_FAILED_INVALID_COMMAND, "XBee 0 AT response failed: invalid command", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_XBEE1_AT_RESPONSE_FAILED_INVALID_COMMAND, "XBee 1 AT response failed: invalid command", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_XBEE0_AT_RESPONSE_FAILED_INVALID_PARAMETER, "XBee 0 AT response failed: invalid parameter", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_XBEE1_AT_RESPONSE_FAILED_INVALID_PARAMETER, "XBee 1 AT response failed: invalid parameter", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_XBEE0_RESET_FAILED, "XBee 0 failed to reset", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_XBEE1_RESET_FAILED, "XBee 1 failed to reset", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_XBEE0_ENABLE_RTS_FAILED, "XBee 0 failed to enable RTS", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_XBEE1_ENABLE_RTS_FAILED, "XBee 1 failed to enable RTS", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_XBEE0_GET_FW_VERSION_FAILED, "XBee 0 failed to get firmware version", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_XBEE1_GET_FW_VERSION_FAILED, "XBee 1 failed to get firmware version", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_XBEE0_SET_NODE_ID_FAILED, "XBee 0 failed to set text node ID", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_XBEE1_SET_NODE_ID_FAILED, "XBee 1 failed to set text node ID", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_XBEE0_SET_CHANNEL_FAILED, "XBee 0 failed to set radio channel", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_XBEE1_SET_CHANNEL_FAILED, "XBee 1 failed to set radio channel", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_XBEE0_SET_PAN_ID_FAILED, "XBee 0 failed to set PAN ID", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_XBEE1_SET_PAN_ID_FAILED, "XBee 1 failed to set PAN ID", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_XBEE0_SET_ADDRESS_FAILED, "XBee 0 failed to set 16-bit address", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_XBEE1_SET_ADDRESS_FAILED, "XBee 1 failed to set 16-bit address", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_OUT_MICROPACKET_OVERFLOW, "Outbound micropacket overflow", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_OUT_MICROPACKET_NOPIPE, "Outbound micropacket to invalid pipe", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_OUT_MICROPACKET_BAD_LENGTH, "Outbound micropacket invalid length", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_DEBUG_OVERFLOW, "Debug endpoint overflow", Annunciator::Message::TRIGGER_EDGE },
	};

	const FaultMessageInfo<XBeeDongle::DongleFault> DONGLE_FAULT_MESSAGE_INFOS[] = {
		{ XBeeDongle::FAULT_ERROR_QUEUE_OVERFLOW, "Local error queue overflow", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT1, "Failed to send to robot 1", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT2, "Failed to send to robot 2", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT3, "Failed to send to robot 3", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT4, "Failed to send to robot 4", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT5, "Failed to send to robot 5", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT6, "Failed to send to robot 6", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT7, "Failed to send to robot 7", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT8, "Failed to send to robot 8", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT9, "Failed to send to robot 9", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT10, "Failed to send to robot 10", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT11, "Failed to send to robot 11", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT12, "Failed to send to robot 12", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT13, "Failed to send to robot 13", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT14, "Failed to send to robot 14", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_SEND_FAILED_ROBOT15, "Failed to send to robot 15", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT1, "Inbound micropacket overflow from robot 1", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT2, "Inbound micropacket overflow from robot 2", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT3, "Inbound micropacket overflow from robot 3", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT4, "Inbound micropacket overflow from robot 4", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT5, "Inbound micropacket overflow from robot 5", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT6, "Inbound micropacket overflow from robot 6", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT7, "Inbound micropacket overflow from robot 7", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT8, "Inbound micropacket overflow from robot 8", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT9, "Inbound micropacket overflow from robot 9", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT10, "Inbound micropacket overflow from robot 10", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT11, "Inbound micropacket overflow from robot 11", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT12, "Inbound micropacket overflow from robot 12", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT13, "Inbound micropacket overflow from robot 13", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT14, "Inbound micropacket overflow from robot 14", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_OVERFLOW_ROBOT15, "Inbound micropacket overflow from robot 15", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT1, "Inbound micropacket to invalid pipe from robot 1", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT2, "Inbound micropacket to invalid pipe from robot 2", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT3, "Inbound micropacket to invalid pipe from robot 3", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT4, "Inbound micropacket to invalid pipe from robot 4", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT5, "Inbound micropacket to invalid pipe from robot 5", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT6, "Inbound micropacket to invalid pipe from robot 6", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT7, "Inbound micropacket to invalid pipe from robot 7", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT8, "Inbound micropacket to invalid pipe from robot 8", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT9, "Inbound micropacket to invalid pipe from robot 9", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT10, "Inbound micropacket to invalid pipe from robot 10", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT11, "Inbound micropacket to invalid pipe from robot 11", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT12, "Inbound micropacket to invalid pipe from robot 12", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT13, "Inbound micropacket to invalid pipe from robot 13", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT14, "Inbound micropacket to invalid pipe from robot 14", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_NOPIPE_ROBOT15, "Inbound micropacket to invalid pipe from robot 15", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT1, "Inbound micropacket invalid length from robot 1", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT2, "Inbound micropacket invalid length from robot 2", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT3, "Inbound micropacket invalid length from robot 3", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT4, "Inbound micropacket invalid length from robot 4", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT5, "Inbound micropacket invalid length from robot 5", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT6, "Inbound micropacket invalid length from robot 6", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT7, "Inbound micropacket invalid length from robot 7", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT8, "Inbound micropacket invalid length from robot 8", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT9, "Inbound micropacket invalid length from robot 9", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT10, "Inbound micropacket invalid length from robot 10", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT11, "Inbound micropacket invalid length from robot 11", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT12, "Inbound micropacket invalid length from robot 12", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT13, "Inbound micropacket invalid length from robot 13", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT14, "Inbound micropacket invalid length from robot 14", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_IN_MICROPACKET_BAD_LENGTH_ROBOT15, "Inbound micropacket invalid length from robot 15", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_BTSEF, "Bit stuff error", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_BTOEF, "Bus turnaround timeout", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_DFN8EF, "Non-integral data field byte count", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_CRC16EF, "CRC16 error", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_CRC5EF, "CRC5 error", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_PIDEF, "PID check error", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_STALL, "Stall handshake returned", Annunciator::Message::TRIGGER_EDGE },
	};

	const FaultMessageInfo<XBeeDongle::RobotFault> ROBOT_FAULT_MESSAGE_INFOS[] = {
		{ XBeeDongle::FAULT_CAPACITOR_CHARGE_TIMEOUT, "Capacitor charge timeout", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_CHICKER_COMM_ERROR, "Chicker communication error", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_CHICKER_NOT_PRESENT, "Chicker not present", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_FPGA_NO_BITSTREAM, "FPGA no bitstream", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_FPGA_INVALID_BITSTREAM, "FPGA invalid bitstream", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_FPGA_DCM_LOCK_FAILED, "FPGA DCM lock failed", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_FPGA_ONLINE_CRC_FAILED, "FPGA online fabric CRC failed", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_FPGA_COMM_ERROR, "FPGA communication error", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_OSCILLATOR_FAILED, "Oscillator failed", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_MOTOR1_HALL_000, "Motor 1 Hall sensor invalid code 000", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_MOTOR2_HALL_000, "Motor 2 Hall sensor invalid code 000", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_MOTOR3_HALL_000, "Motor 3 Hall sensor invalid code 000", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_MOTOR4_HALL_000, "Motor 4 Hall sensor invalid code 000", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_MOTORD_HALL_000, "Motor D Hall sensor invalid code 000", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_MOTOR1_HALL_111, "Motor 1 Hall sensor invalid code 111", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_MOTOR2_HALL_111, "Motor 2 Hall sensor invalid code 111", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_MOTOR3_HALL_111, "Motor 3 Hall sensor invalid code 111", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_MOTOR4_HALL_111, "Motor 4 Hall sensor invalid code 111", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_MOTORD_HALL_111, "Motor D Hall sensor invalid code 111", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_MOTOR1_HALL_STUCK, "Motor 1 Hall sensor stuck w.r.t. optical encoder", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_MOTOR2_HALL_STUCK, "Motor 2 Hall sensor stuck w.r.t. optical encoder", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_MOTOR3_HALL_STUCK, "Motor 3 Hall sensor stuck w.r.t. optical encoder", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_MOTOR4_HALL_STUCK, "Motor 4 Hall sensor stuck w.r.t. optical encoder", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_MOTOR1_ENCODER_STUCK, "Motor 1 optical encoder stuck w.r.t. Hall sensor", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_MOTOR2_ENCODER_STUCK, "Motor 2 optical encoder stuck w.r.t. Hall sensor", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_MOTOR3_ENCODER_STUCK, "Motor 3 optical encoder stuck w.r.t. Hall sensor", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_MOTOR4_ENCODER_STUCK, "Motor 4 optical encoder stuck w.r.t. Hall sensor", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_DRIBBLER_OVERHEAT, "Dribbler overheating", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_DRIBBLER_THERMISTOR_FAILED, "Dribbler thermistor communication error", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_FLASH_COMM_ERROR, "Flash communication error", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_FLASH_PARAMS_CORRUPT, "Flash operational parameters block corrupt", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_NO_PARAMS, "Flash operational parameters block absent", Annunciator::Message::TRIGGER_LEVEL },
		{ XBeeDongle::FAULT_IN_PACKET_OVERFLOW, "Inbound packet overflow", Annunciator::Message::TRIGGER_EDGE },
		{ XBeeDongle::FAULT_FIRMWARE_BAD_REQUEST, "Bad firmware request", Annunciator::Message::TRIGGER_EDGE },
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

	Annunciator::Message unknown_local_error_message("Dongle local error queue contained unknown error code", Annunciator::Message::TRIGGER_EDGE);
	Annunciator::Message in_micropacket_overflow_message("Inbound micropacket overflow from dongle", Annunciator::Message::TRIGGER_EDGE);

	enum TBotsControlRequest {
		TBOTS_CONTROL_REQUEST_GET_XBEE_FW_VERSION = 0x00,
		TBOTS_CONTROL_REQUEST_GET_XBEE_CHANNELS = 0x01,
		TBOTS_CONTROL_REQUEST_ENABLE_RADIOS = 0x02,
	};

	XBeeDongle::EStopState decode_estop_state(uint8_t st) {
		XBeeDongle::EStopState est = static_cast<XBeeDongle::EStopState>(st);
		switch (est) {
			case XBeeDongle::ESTOP_STATE_UNINITIALIZED:
			case XBeeDongle::ESTOP_STATE_DISCONNECTED:
			case XBeeDongle::ESTOP_STATE_STOP:
			case XBeeDongle::ESTOP_STATE_RUN:
				return est;
		}
		throw std::runtime_error("Dongle status illegal emergency stop state");
	}

	XBeeDongle::XBeesState decode_xbees_state(uint8_t st) {
		XBeeDongle::XBeesState est = static_cast<XBeeDongle::XBeesState>(st);
		switch (est) {
			case XBeeDongle::XBEES_STATE_PREINIT:
			case XBeeDongle::XBEES_STATE_INIT1_0:
			case XBeeDongle::XBEES_STATE_INIT1_1:
			case XBeeDongle::XBEES_STATE_INIT1_DONE:
			case XBeeDongle::XBEES_STATE_INIT2_0:
			case XBeeDongle::XBEES_STATE_INIT2_1:
			case XBeeDongle::XBEES_STATE_RUNNING:
			case XBeeDongle::XBEES_STATE_FAIL_0:
			case XBeeDongle::XBEES_STATE_FAIL_1:
				return est;
		}
		throw std::runtime_error("Dongle status illegal XBees state");
	}

	class EnableOperation : public AsyncOperation<void>, public sigc::trackable {
		public:
			static Ptr create(XBeeDongle &dongle, LibUSBDeviceHandle &device, unsigned int out_channel, unsigned int in_channel) {
				Ptr p(new EnableOperation(dongle, device, out_channel, in_channel));
				return p;
			}

			void result() const {
				if (failed_op.is()) {
					failed_op->result();
				} else if (dongle.xbees_state == XBeeDongle::XBEES_STATE_FAIL_0) {
					throw std::runtime_error("Failed to initialize XBee 0");
				} else if (dongle.xbees_state == XBeeDongle::XBEES_STATE_FAIL_1) {
					throw std::runtime_error("Failed to initialize XBee 1");
				}
			}

		private:
			XBeeDongle &dongle;
			LibUSBDeviceHandle &device;
			unsigned int out_channel, in_channel;
			sigc::connection xbees_state_connection;
			AsyncOperation<void>::Ptr failed_op;
			int libusb_error;
			Ptr self_ref;

			EnableOperation(XBeeDongle &dongle, LibUSBDeviceHandle &device, unsigned int out_channel, unsigned int in_channel) : dongle(dongle), device(device), out_channel(out_channel), in_channel(in_channel), libusb_error(LIBUSB_SUCCESS), self_ref(this) {
				xbees_state_connection = dongle.xbees_state.signal_changed().connect(sigc::mem_fun(this, &EnableOperation::on_xbees_state_changed));
				on_xbees_state_changed();
			}

			~EnableOperation() {
			}

			void on_xbees_state_changed() {
				switch (dongle.xbees_state) {
					case XBeeDongle::XBEES_STATE_PREINIT:
					case XBeeDongle::XBEES_STATE_INIT1_0:
					case XBeeDongle::XBEES_STATE_INIT1_1:
						return;

					case XBeeDongle::XBEES_STATE_INIT1_DONE:
						send_enable_radios_message();
						return;

					case XBeeDongle::XBEES_STATE_INIT2_0:
					case XBeeDongle::XBEES_STATE_INIT2_1:
						return;

					case XBeeDongle::XBEES_STATE_RUNNING:
					case XBeeDongle::XBEES_STATE_FAIL_0:
					case XBeeDongle::XBEES_STATE_FAIL_1:
					{
						xbees_state_connection.disconnect();
						Ptr pthis(this);
						self_ref.reset();
						signal_done.emit(pthis);
						return;
					}
				}
			}

			void send_enable_radios_message() {
				LibUSBControlNoDataTransfer::Ptr transfer = LibUSBControlNoDataTransfer::create(device, 0x40, 0x02, static_cast<uint16_t>((in_channel << 8) | out_channel), 0x0000, 0, 0);
				transfer->signal_done.connect(sigc::mem_fun(this, &EnableOperation::on_enable_radios_done));
				transfer->submit();
			}

			void on_enable_radios_done(AsyncOperation<void>::Ptr op) {
				if (!op->succeeded()) {
					xbees_state_connection.disconnect();
					failed_op = op;
					Ptr pthis(this);
					self_ref.reset();
					signal_done.emit(pthis);
				}
			}
	};
}

XBeeDongle::XBeeDongle(unsigned int out_channel, unsigned int in_channel) : estop_state(ESTOP_STATE_UNINITIALIZED), xbees_state(XBEES_STATE_PREINIT), out_channel(out_channel), in_channel(in_channel), context(), device(context, 0x04D8, 0x7839), dirty_drive_mask(0) {
	for (unsigned int i = 0; i < G_N_ELEMENTS(robots); ++i) {
		robots[i] = XBeeRobot::create(*this, i + 1);
	}
	device.set_configuration(1)->result();
	device.claim_interface(0)->result();
	device.claim_interface(1)->result();
	device.claim_interface(2)->result();

	LibUSBInterruptInTransfer::Ptr dongle_status_transfer = LibUSBInterruptInTransfer::create(device, EP_DONGLE_STATUS | 0x80, 4, true, 0, STALL_LIMIT);
	dongle_status_transfer->signal_done.connect(sigc::bind(sigc::mem_fun(this, &XBeeDongle::on_dongle_status), dongle_status_transfer));
	dongle_status_transfer->repeat(true);
	dongle_status_transfer->submit();

	LibUSBInterruptInTransfer::Ptr local_error_queue_transfer = LibUSBInterruptInTransfer::create(device, EP_LOCAL_ERROR_QUEUE | 0x80, 64, false, 0, STALL_LIMIT);
	local_error_queue_transfer->signal_done.connect(sigc::bind(sigc::mem_fun(this, &XBeeDongle::on_local_error_queue), local_error_queue_transfer));
	local_error_queue_transfer->repeat(true);
	local_error_queue_transfer->submit();

	LibUSBInterruptInTransfer::Ptr debug_transfer = LibUSBInterruptInTransfer::create(device, EP_DEBUG | 0x80, 4096, false, 0, STALL_LIMIT);
	debug_transfer->signal_done.connect(sigc::bind(sigc::mem_fun(this, &XBeeDongle::on_debug), debug_transfer));
	debug_transfer->repeat(true);
	debug_transfer->submit();
}

XBeeDongle::~XBeeDongle() {
}

AsyncOperation<void>::Ptr XBeeDongle::enable() {
	EnableOperation::Ptr p = EnableOperation::create(*this, device, out_channel, in_channel);
	p->signal_done.connect(sigc::mem_fun(this, &XBeeDongle::on_enable_done));
	return p;
}

#warning function is badly named
AsyncOperation<void>::Ptr XBeeDongle::send_bulk(const void *data, std::size_t length) {
	LibUSBInterruptOutTransfer::Ptr transfer = LibUSBInterruptOutTransfer::create(device, EP_INTERRUPT, data, length, 0, STALL_LIMIT);
	transfer->submit();
	return transfer;
}

void XBeeDongle::on_enable_done(AsyncOperation<void>::Ptr op) {
	if (op->succeeded()) {
		LibUSBInterruptInTransfer::Ptr transfer = LibUSBInterruptInTransfer::create(device, EP_STATE_TRANSPORT | 0x80, 64, false, 0, STALL_LIMIT);
		transfer->signal_done.connect(sigc::bind(sigc::mem_fun(this, &XBeeDongle::on_state_transport_in), transfer));
		transfer->repeat(true);
		transfer->submit();

		transfer = LibUSBInterruptInTransfer::create(device, EP_INTERRUPT | 0x80, 64, false, 0, STALL_LIMIT);
		transfer->signal_done.connect(sigc::bind(sigc::mem_fun(this, &XBeeDongle::on_interrupt_in), transfer));
		transfer->repeat(true);
		transfer->submit();

		stamp_connection = Glib::signal_timeout().connect(sigc::bind_return(sigc::mem_fun(this, &XBeeDongle::on_stamp), true), 300);
	}
}

void XBeeDongle::on_dongle_status(AsyncOperation<void>::Ptr, LibUSBInterruptInTransfer::Ptr transfer) {
	transfer->result();
	estop_state = decode_estop_state(transfer->data()[0]);
	xbees_state = decode_xbees_state(transfer->data()[1]);
	uint16_t mask = static_cast<uint16_t>(transfer->data()[2] | (transfer->data()[3] << 8));
	for (unsigned int i = 1; i <= 15; ++i) {
		XBeeRobot::Ptr bot = robot(i);
		bot->alive = !!(mask & (1 << i));
	}
}

void XBeeDongle::on_local_error_queue(AsyncOperation<void>::Ptr, LibUSBInterruptInTransfer::Ptr transfer) {
	transfer->result();
	for (std::size_t i = 0; i < transfer->size(); ++i) {
		uint8_t code = transfer->data()[i];
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
}

void XBeeDongle::on_state_transport_in(AsyncOperation<void>::Ptr, LibUSBInterruptInTransfer::Ptr transfer) {
	transfer->result();
	for (std::size_t i = 0; i < transfer->size(); i += transfer->data()[i]) {
		assert(i + transfer->data()[i] <= transfer->size());
		unsigned int index = transfer->data()[i + 1] >> 4;
		assert(1 <= index && index <= 15);
		unsigned int pipe = transfer->data()[i + 1] & 0x0F;
		assert(pipe == PIPE_FEEDBACK);
		robot(index)->on_feedback(transfer->data() + i + 2, transfer->data()[i] - 2);
	}
}

void XBeeDongle::on_interrupt_in(AsyncOperation<void>::Ptr, LibUSBInterruptInTransfer::Ptr transfer) {
	transfer->result();
	if (transfer->size()) {
		unsigned int robot = transfer->data()[0] >> 4;
		Pipe pipe = static_cast<Pipe>(transfer->data()[0] & 0x0F);
		signal_interrupt_message_received.emit(robot, pipe, transfer->data() + 1, transfer->size() - 1);
	}
}

void XBeeDongle::on_debug(AsyncOperation<void>::Ptr, LibUSBInterruptInTransfer::Ptr transfer) {
	try {
		transfer->result();
		if (transfer->size()) {
			std::string str(reinterpret_cast<const char *>(transfer->data()), transfer->size());
			LOG_INFO(Glib::locale_to_utf8(str));
		}
	} catch (const LibUSBTransferError &err) {
		LOG_ERROR(Glib::ustring::compose("Ignoring %1", err.what()));
	}
}

void XBeeDongle::on_stamp() {
	LibUSBInterruptOutTransfer::Ptr transfer = LibUSBInterruptOutTransfer::create(device, EP_STATE_TRANSPORT, 0, 0, 0, STALL_LIMIT);
	transfer->signal_done.connect(&discard_result);
	transfer->submit();
}

void XBeeDongle::dirty_drive(unsigned int index) {
	assert(1 <= index && index <= 15);
	dirty_drive_mask |= 1 << index;
	if (!flush_drive_connection) {
		flush_drive_connection = Glib::signal_idle().connect(sigc::bind_return(sigc::mem_fun(this, &XBeeDongle::flush_drive), false));
	}
}

void XBeeDongle::flush_drive() {
	uint8_t buffer[64 / (10 + 2)][10 + 2];
	std::size_t wptr = 0;

	for (unsigned int i = 1; i <= 15; ++i) {
		if (dirty_drive_mask & (1 << i)) {
			dirty_drive_mask &= ~(1 << i);
			buffer[wptr][0] = sizeof(buffer[0]);
			buffer[wptr][1] = static_cast<uint8_t>(i << 4) | PIPE_DRIVE;
			std::memcpy(&buffer[wptr][2], robot(i)->drive_block, sizeof(buffer[0]) - 2);
			++wptr;

			if (wptr == sizeof(buffer) / sizeof(*buffer)) {
				LibUSBInterruptOutTransfer::Ptr transfer = LibUSBInterruptOutTransfer::create(device, EP_STATE_TRANSPORT, buffer, wptr * sizeof(buffer[0]), 0, STALL_LIMIT);
				transfer->signal_done.connect(&discard_result);
				transfer->submit();
				wptr = 0;
			}
		}
	}

	if (wptr) {
		LibUSBInterruptOutTransfer::Ptr transfer = LibUSBInterruptOutTransfer::create(device, EP_STATE_TRANSPORT, buffer, wptr * sizeof(buffer[0]), 0, STALL_LIMIT);
		transfer->signal_done.connect(&discard_result);
		transfer->submit();
	}

	stamp_connection.disconnect();
	stamp_connection = Glib::signal_timeout().connect(sigc::bind_return(sigc::mem_fun(this, &XBeeDongle::on_stamp), true), 300);
}

