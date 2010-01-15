#ifndef XBEE_PACKETTYPES_H
#define XBEE_PACKETTYPES_H

#include <cstddef>
#include <stdint.h>

namespace xbeepacket {
	template<std::size_t T_value_length>
	struct __attribute__((packed)) AT_REQUEST {
		uint8_t apiid;
		uint8_t frame;
		uint8_t command[2];
		uint8_t value[T_value_length];
	};
	const uint8_t AT_REQUEST_APIID = 0x08;

	struct __attribute__((packed)) AT_RESPONSE {
		uint8_t apiid;
		uint8_t frame;
		uint8_t command[2];
		uint8_t status;
		uint8_t value[];
	};
	const uint8_t AT_RESPONSE_APIID = 0x88;
	const uint8_t AT_RESPONSE_STATUS_OK = 0;
	const uint8_t AT_RESPONSE_STATUS_ERROR = 1;
	const uint8_t AT_RESPONSE_STATUS_INVALID_COMMAND = 2;
	const uint8_t AT_RESPONSE_STATUS_INVALID_PARAMETER = 3;

	template<std::size_t T_value_length>
	struct __attribute__((packed)) REMOTE_AT_REQUEST {
		uint8_t apiid;
		uint8_t frame;
		uint8_t address64[8];
		uint8_t address16[2];
		uint8_t options;
		uint8_t command[2];
		uint8_t value[T_value_length];
	};
	const uint8_t REMOTE_AT_REQUEST_APIID = 0x17;
	const uint8_t REMOTE_AT_REQUEST_OPTION_APPLY = 0x02;

	struct __attribute__((packed)) REMOTE_AT_RESPONSE {
		uint8_t apiid;
		uint8_t frame;
		uint8_t address64[8];
		uint8_t address16[2];
		uint8_t command[2];
		uint8_t status;
		uint8_t value[];
	};
	const uint8_t REMOTE_AT_RESPONSE_APIID = 0x97;
	const uint8_t REMOTE_AT_RESPONSE_STATUS_OK = 0;
	const uint8_t REMOTE_AT_RESPONSE_STATUS_ERROR = 1;
	const uint8_t REMOTE_AT_RESPONSE_STATUS_INVALID_COMMAND = 2;
	const uint8_t REMOTE_AT_RESPONSE_STATUS_INVALID_PARAMETER = 3;
	const uint8_t REMOTE_AT_RESPONSE_STATUS_NO_RESPONSE = 4;

	struct __attribute__((packed)) TRANSMIT_HDR {
		uint8_t apiid;
		uint8_t frame;
		uint8_t address[8];
		uint8_t options;
	};
	const uint8_t TRANSMIT_APIID = 0x00;
	const uint8_t TRANSMIT_OPTION_DISABLE_ACK = 0x01;
	const uint8_t TRANSMIT_OPTION_BROADCAST_PANID = 0x04;

	struct __attribute__((packed)) TRANSMIT_STATUS {
		uint8_t apiid;
		uint8_t frame;
		uint8_t status;
	};
	const uint8_t TRANSMIT_STATUS_APIID = 0x89;
	const uint8_t TRANSMIT_STATUS_SUCCESS = 0;
	const uint8_t TRANSMIT_STATUS_NO_ACK = 1;
	const uint8_t TRANSMIT_STATUS_NO_CCA = 2;
	const uint8_t TRANSMIT_STATUS_PURGED = 3;

	struct __attribute__((packed)) RECEIVE_HDR {
		uint8_t apiid;
		uint8_t address[8];
		uint8_t rssi;
		uint8_t options;
	};
	const uint8_t RECEIVE_APIID = 0x80;
	const uint8_t RECEIVE_OPTION_BROADCAST_ADDRESS = 0x02;
	const uint8_t RECEIVE_OPTION_BROADCAST_PANID = 0x04;

	struct __attribute__((packed)) FEEDBACK_DATA {
		RECEIVE_HDR rxhdr;
		uint8_t flags;
		uint8_t outbound_rssi;
		uint16_t dribbler_speed;
		uint16_t battery_level;
		uint8_t faults;
		uint8_t command_ack;
	};
	const uint8_t FEEDBACK_FLAG_RUNNING = 0x80;

	struct __attribute__((packed)) RUN_DATA {
		TRANSMIT_HDR txhdr;
		uint8_t flags;
		int16_t drive_speeds[4];
		int16_t dribbler_speed;
		uint8_t command_seq;
		uint8_t command;
		uint16_t command_data;
	};
	const uint8_t RUN_FLAG_RUNNING = 0x80;
	const uint8_t RUN_FLAG_DIRECT_DRIVE = 0x01;
	const uint8_t RUN_FLAG_CONTROLLED_DRIVE = 0x02;
	const uint8_t RUN_FLAG_DRIBBLE = 0x04;
	const uint8_t RUN_FLAG_FEEDBACK = 0x40;
	const uint8_t RUN_COMMAND_NOOP = 0x00;
	const uint8_t RUN_COMMAND_KICK = 0x01;
}

#endif

