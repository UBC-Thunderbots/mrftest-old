#ifndef PARBUS_H
#define PARBUS_H

/**
 * \file
 *
 * \brief Defines the structures sent and received over the parallel bus.
 */

#include <stdint.h>

/**
 * \brief The flags byte of an outbound parallel bus packet.
 */
typedef struct {
	unsigned motors_direction : 5;
	unsigned kick_sequence : 1;
	unsigned enable_charger : 1;
	unsigned enable_motors : 1;
} parbus_txpacket_flags_t;

/**
 * \brief The possible test modes to specify in an outbound parallel bus packet.
 */
typedef enum {
	PARBUS_TXPACKET_TEST_MODE_NONE,
	PARBUS_TXPACKET_TEST_MODE_LAMPTEST,
	PARBUS_TXPACKET_TEST_MODE_HALL,
	PARBUS_TXPACKET_TEST_MODE_ENCODER_LINES,
	PARBUS_TXPACKET_TEST_MODE_ENCODER_COUNT,
	PARBUS_TXPACKET_TEST_MODE_BOOSTCONVERTER,
} parbus_txpacket_test_mode_t;

/**
 * \brief The test setup byte of an outbound parallel bus packet.
 */
typedef struct {
	unsigned index : 4;
	parbus_txpacket_test_mode_t mode : 4;
} parbus_txpacket_test_t;

/**
 * \brief A packet sent over the parallel bus to the FPGA.
 */
typedef struct {
	parbus_txpacket_flags_t flags;
	uint8_t motors_power[5];
	uint16_t battery_voltage;
	parbus_txpacket_test_t test;
	uint16_t kick_power;
} parbus_txpacket_t;

/**
 * \brief The flags byte of an inbound parallel bus packet.
 */
typedef struct {
	unsigned feedback_ok : 1;
	unsigned chicker_present : 1;
	unsigned : 6;
} parbus_rxpacket_flags_t;

/**
 * \brief A packet received over the parallel bus from the FPGA.
 */
typedef struct {
	parbus_rxpacket_flags_t flags;
	uint16_t capacitor_voltage;
	int16_t encoders_diff[4];
} parbus_rxpacket_t;

#endif

