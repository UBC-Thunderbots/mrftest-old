#ifndef XBEE_PACKETTYPES_H
#define XBEE_PACKETTYPES_H

#include <cstddef>
#include <stdint.h>

namespace xbeepacket {
	template<std::size_t T_value_length>
	struct REMOTE_AT_REQUEST {
		uint8_t apiid;
		uint8_t frame;
		uint8_t address64[8];
		uint8_t address16[2];
		uint8_t options;
		uint8_t command[2];
		uint8_t value[T_value_length];
	} __attribute__((__packed__));
	const uint8_t REMOTE_AT_REQUEST_APIID = 0x17;
	const uint8_t REMOTE_AT_REQUEST_OPTION_APPLY = 0x02;

	template<std::size_t T_value_length>
	struct REMOTE_AT_RESPONSE {
		uint8_t apiid;
		uint8_t frame;
		uint8_t address64[8];
		uint8_t address16[2];
		uint8_t command[2];
		uint8_t status;
		uint8_t value[T_value_length];
	} __attribute__((__packed__));
	const uint8_t REMOTE_AT_RESPONSE_APIID = 0x97;
	const uint8_t REMOTE_AT_RESPONSE_STATUS_OK = 0;
	const uint8_t REMOTE_AT_RESPONSE_STATUS_ERROR = 1;
	const uint8_t REMOTE_AT_RESPONSE_STATUS_INVALID_COMMAND = 2;
	const uint8_t REMOTE_AT_RESPONSE_STATUS_INVALID_PARAMETER = 3;
	const uint8_t REMOTE_AT_RESPONSE_STATUS_NO_RESPONSE = 4;

	template<typename T_value>
	struct TRANSMIT {
		uint8_t apiid;
		uint8_t frame;
		uint8_t address[8];
		uint8_t options;
		T_value data;
	} __attribute__((__packed__));
	const uint8_t TRANSMIT_APIID = 0x00;
	const uint8_t TRANSMIT_OPTION_DISABLE_ACK = 0x01;
	const uint8_t TRANSMIT_OPTION_BROADCAST_PANID = 0x04;

	template<typename T_value>
	struct RECEIVE {
		uint8_t apiid;
		uint8_t address[8];
		uint8_t rssi;
		uint8_t options;
		T_value data;
	} __attribute__((__packed__));
	const uint8_t RECEIVE_APIID = 0x80;
	const uint8_t RECEIVE_OPTION_BROADCAST_ADDRESS = 0x02;
	const uint8_t RECEIVE_OPTION_BROADCAST_PANID = 0x04;
}

#endif

