#ifndef XBEE_SHARED_PACKETTYPES_H
#define XBEE_SHARED_PACKETTYPES_H

#include <cstddef>
#include <ctime>
#include <stdint.h>
#include <pthread.h>

namespace xbeepacket {
	template<std::size_t T_value_length>
	struct __attribute__((packed)) AT_REQUEST {
		uint8_t apiid;
		uint8_t frame;
		uint8_t command[2];
		uint8_t value[T_value_length];
	};
	const uint8_t AT_REQUEST_APIID = 0x08;
	const uint8_t AT_REQUEST_QUEUE_APIID = 0x09;

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

	template<std::size_t value_size>
	struct __attribute__((packed)) REMOTE_AT_REQUEST {
		uint8_t apiid;
		uint8_t frame;
		uint8_t address64[8];
		uint8_t address16[2];
		uint8_t options;
		uint8_t command[2];
		uint8_t value[value_size];
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

	struct __attribute__((packed)) TRANSMIT64_HDR {
		uint8_t apiid;
		uint8_t frame;
		uint8_t address[8];
		uint8_t options;
	};
	const uint8_t TRANSMIT64_APIID = 0x00;

	struct __attribute__((packed)) TRANSMIT16_HDR {
		uint8_t apiid;
		uint8_t frame;
		uint8_t address[2];
		uint8_t options;
	};
	const uint8_t TRANSMIT16_APIID = 0x01;
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

	struct __attribute__((packed)) RECEIVE64_HDR {
		uint8_t apiid;
		uint8_t address[8];
		uint8_t rssi;
		uint8_t options;
	};
	const uint8_t RECEIVE64_APIID = 0x80;

	struct __attribute__((packed)) RECEIVE16_HDR {
		uint8_t apiid;
		uint8_t address[2];
		uint8_t rssi;
		uint8_t options;
	};
	const uint8_t RECEIVE16_APIID = 0x81;
	const uint8_t RECEIVE_OPTION_BROADCAST_ADDRESS = 0x02;
	const uint8_t RECEIVE_OPTION_BROADCAST_PANID = 0x04;

	const unsigned int MAX_PAYLOAD = 100;

	struct __attribute__((packed)) FEEDBACK_DATA {
		uint8_t flags;
		uint8_t outbound_rssi;
		uint16_t dribbler_speed;
		uint16_t battery_level;
		uint8_t faults;
	};
	const uint8_t FEEDBACK_FLAG_RUNNING = 0x80;
	const uint8_t FEEDBACK_FLAG_CHICKER_READY = 0x01;
	const uint8_t FEEDBACK_FLAG_CHICKER_FAULT = 0x02;

	struct __attribute__((packed)) RUN_DATA {
		uint8_t flags;
		signed drive1_speed : 11;
		signed drive2_speed : 11;
		signed drive3_speed : 11;
		signed drive4_speed : 11;
		unsigned dribbler_speed : 11;
		unsigned chick_power : 9;
	};
	const uint8_t RUN_FLAG_RUNNING = 0x80;
	const uint8_t RUN_FLAG_DIRECT_DRIVE = 0x01;
	const uint8_t RUN_FLAG_CONTROLLED_DRIVE = 0x02;
	const uint8_t RUN_FLAG_CHICKER_ENABLED = 0x04;
	const uint8_t RUN_FLAG_CHIP = 0x08;
	const uint8_t RUN_FLAG_FEEDBACK = 0x40;

	//
	// The maximum number of robots that can be driving at a time, based on the
	// amount of space available in a packet payload and the size of a run data
	// structure.
	//
	const unsigned int MAX_DRIVE_ROBOTS = MAX_PAYLOAD / sizeof(RUN_DATA);

	//
	// This is the format of a block of data in the shared memory area. To avoid
	// runaway robots, the client is required to update the "timestamp" field
	// with the current time each time it updates the run data structure. The
	// arbiter will check the value of the "timestamp" field; if it is more than
	// half a second old, the arbiter will scram the robot.
	//
	struct __attribute__((packed)) SHM_FRAME {
		RUN_DATA run_data;
		FEEDBACK_DATA feedback_data;
		timespec timestamp;
		uint64_t delivery_mask;
		timespec latency;
		uint8_t inbound_rssi;
	};

	//
	// This is the format of the entire shared memory area.
	//
	struct __attribute__((packed)) SHM_BLOCK {
		pthread_rwlock_t lock;
		SHM_FRAME frames[MAX_DRIVE_ROBOTS];
	};

	//
	// META packets are special packets that are not sent to the XBee; instead,
	// they are used to communicate with the arbiter for configuration. A META
	// packet starts with a META_HDR with apiid set to META_APIID and metatype
	// identifying which type of META packet is being transmitted.
	//
	struct __attribute__((packed)) META_HDR {
		uint8_t apiid;
		uint8_t metatype;
	};
	const uint8_t META_APIID = 0x7E;

	//
	// A META_CLAIM packet is sent from a client to the arbiter when the client
	// wishes to begin communicating with a robot. The "address" field contains
	// the 64-bit address of the robot the client wishes to claim.
	//
	// If "drive_mode" is nonzero, the client is requesting that the robot be
	// operated in regular driving mode. The arbiter will attempt to allocate
	// and assign a 16-bit address, an offset in the run data packet, and a
	// region of shared memory. The arbiter will also track the power state of
	// the robot, sending META_ALIVE and META_DEAD packets when the robot starts
	// and stops responding. The client will then use the shared memory block to
	// push drive updates and read feedback data.
	//
	// If "drive_mode" is zero, the client is requesting that the robot be
	// operated in "raw mode". The arbiter will not allocate any resources nor
	// track the robot's power state. The client will send and receive unicast
	// packets directly over the Unix-domain socket to talk to the robot. It is
	// expected that this mode will be used for bootloading and emergency
	// erasing of Flash memory.
	//
	// The arbiter will first determine whether the claim request should succeed
	// based on whether any other client has already claimed the robot and
	// whether the necessary resources can be allocated.
	//
	// If the claim request fails outright, a META_CLAIM_FAILED will be returned
	// immediately indicating the reason for the failure.
	//
	// If the claim request is acceptable, the robot will be claimed for this
	// client. If "drive_mode" is zero, a META_ALIVE will be sent immediately.
	// If "drive_mode" is nonzero, no response is sent immediately, but the
	// arbiter will begin scanning for the presence of the robot and attempting
	// to configure it; once the robot is configured, a META_ALIVE will then be
	// sent.
	//
	struct __attribute__((packed)) META_CLAIM {
		META_HDR hdr;
		uint64_t address;
		uint8_t drive_mode;
	};
	const uint8_t CLAIM_METATYPE = 0x00;

	//
	// A META_CLAIM_FAILED packet is sent from the arbiter to a client in
	// response to a prior META_CLAIM packet if the claim request failed because
	// the robot was already claimed or because the claim request required that
	// resources be allocated and the required resources were not available.
	//
	// CLAIM_FAILED_LOCKED_METATYPE indicates that the robot was already
	// claimed.
	//
	// CLAIM_FAILED_RESOURCE_METATYPE indicates that resources were required but
	// insufficient resources could be allocated.
	//
	struct __attribute__((packed)) META_CLAIM_FAILED {
		META_HDR hdr;
		uint64_t address;
	};
	const uint8_t CLAIM_FAILED_LOCKED_METATYPE = 0x01;
	const uint8_t CLAIM_FAILED_RESOURCE_METATYPE = 0x02;

	//
	// A META_ALIVE packet is sent from the arbiter to a client and indicates
	// that a claimed robot is ready to communicate.
	//
	// If the claim request was for drive mode, the META_ALIVE indicates that
	// the robot is actually powered and running, has accepted its allocated
	// 16-bit address and run data offset, and has returned at least one
	// feedback packet. The client can examine feedback data and send fresh
	// drive data using the shared memory area. In this case, "shm_frame"
	// contains the index of the element of the "frames" array in the shared
	// memory block that has been allocated for this robot, and "address16"
	// contains the 16-bit address that the robot has actually accepted.
	//
	// If the claim request was for raw mode, the META_ALIVE merely acts as an
	// acknowledgement that the META_CLAIM was received and accepted; it says
	// nothing about the actual power state of the robot. The client is expected
	// to attempt communication with the robot to determine its state. In this
	// case, "shm_frame" is 0xFF and "address16" is the 16-bit address that was
	// allocated but not assigned to the robot yet.
	//
	struct __attribute__((packed)) META_ALIVE {
		META_HDR hdr;
		uint64_t address;
		uint16_t address16;
		uint8_t shm_frame;
	};
	const uint8_t ALIVE_METATYPE = 0x03;

	//
	// A META_DEAD packet is sent from the arbiter to a client and indicates
	// that a robot in drive mode that was previously reported as alive (via
	// META_ALIVE) has stopped responding. Robots that were in raw mode do not
	// have their power states tracked by the arbiter, and hence never cause
	// META_DEAD packets to be produced.
	//
	// If the robot returns to life and the client has not unclaimed it, the
	// arbiter will automatically detect the robot and send a META_ALIVE.
	// Communication failure does not cause resource deallocation, so it is
	// guaranteed that once the robot returns to life, sufficient resource will
	// be available to reconfigure it. Further, it is guaranteed that when the
	// robot returns to life, it will be assigned the same frame in the shared
	// memory block (that is, for a given claim of a given robot, the
	// "shm_frame" field in all META_ALIVE packets will be identical).
	//
	struct __attribute__((packed)) META_DEAD {
		META_HDR hdr;
		uint64_t address;
	};
	const uint8_t DEAD_METATYPE = 0x04;

	//
	// A META_FEEDBACK packet is sent from the arbiter to a client and indicates
	// that a robot in drive mode has updated its feedback data. Robots in raw
	// mode do not have their incoming packets interpreted by the arbiter, and
	// hence never cause META_FEEDBACK packet to be produced.
	//
	// The client can find the new feedback data in the shared memory block.
	//
	struct __attribute__((packed)) META_FEEDBACK {
		META_HDR hdr;
		uint64_t address;
	};
	const uint8_t FEEDBACK_METATYPE = 0x05;

	// 
	// A META_RELEASE packet is sent from a client to the arbiter and indicates
	// that the client no longer has any interest in a robot.
	//
	// If the robot was in drive mode, the arbiter will begin trying to deassign
	// resources assigned to the robot (specifically, the run data offset and
	// 16-bit address). If a client attempts to claim the robot in drive mode,
	// the request will succeed immediately; any resources that have not yet
	// been deassigned will be reused, and any resources that have been
	// deassigned will be reallocated. If a client attempts to claim the robot
	// in raw mode, the arbiter will wait until all resources have been
	// deassigned before granting the claim request (if the arbiter is unable to
	// communicate with the robot for a timeout period while trying to deassign
	// resources, it assumes the robot has been powered down and deallocates the
	// resources anyway, thus permitting the raw-mode claim to proceed).
	//
	// If the robot was in raw mode, no communication is triggered. The robot
	// can immediately be claimed by the same or another client in either raw
	// mode or drive mode.
	//
	// The client must assume that any shared memory frame referring to the
	// released robot is immediately invalid.
	//
	// When a client closes its socket connection to the arbiter, all robots
	// claimed by that client are automatically released.
	//
	// It is also permissible to use META_RELEASE to cancel a raw-mode
	// META_CLAIM that has not yet been accepted because the robot still has
	// assigned resources from a prior drive-mode claim.
	//
	struct __attribute__((packed)) META_RELEASE {
		META_HDR hdr;
		uint64_t address;
	};
	const uint8_t RELEASE_METATYPE = 0x06;
}

#endif

