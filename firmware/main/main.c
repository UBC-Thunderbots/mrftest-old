#include "error_reporting.h"
#include "crc.h"
#include "drive.h"
#include "feedback.h"
#include "params.h"
#include "pins.h"
#include "run.h"
#include "serial.h"
#include "signal.h"
#include "spi.h"
#include "xbee_rxpacket.h"
#include "xbee_txpacket.h"
#include <delay.h>
#include <pic18fregs.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

static uint16_t xbee_versions[2];

DEF_INTHIGH(high_handler)
	__asm extern _xbee_rxpacket_rc1if __endasm;
	__asm extern _xbee_rxpacket_rc2if __endasm;
	DEF_HANDLER(SIG_RC1, xbee_rxpacket_rc1if)
	DEF_HANDLER(SIG_RC2, xbee_rxpacket_rc2if)
END_DEF

DEF_INTLOW(low_handler)
	__asm extern _xbee_txpacket_tx1if __endasm;
	__asm extern _xbee_txpacket_tx2if __endasm;
	__asm extern _xbee_txpacket_tmr4if __endasm;
	DEF_HANDLER2(SIG_TX1, SIG_TX1IE, xbee_txpacket_tx1if)
	DEF_HANDLER2(SIG_TX2, SIG_TX2IE, xbee_txpacket_tx2if)
	DEF_HANDLER(SIG_TMR4, xbee_txpacket_tmr4if)
END_DEF

static void checksum_pic_rom(void) {
	__code const uint8_t *ptr = 0;
	uint16_t crc = CRC16_EMPTY;

	while (ptr != (__code const uint8_t *) 0x1F000) {
		crc = crc_update(crc, *ptr++);
	}

	firmware_crc = crc;
}

/**
 * \brief Constructs an AT Command packet and sends it to an XBee.
 *
 * \param[in] xbee the index of the XBee to send the command to.
 *
 * \param[in] frame the frame number to use.
 *
 * \param[in] command the AT command to send.
 *
 * \param[in] value the value to set the parameter to.
 *
 * \param[in] val_length the number of bytes to use to represent \p value.
 */
static void at_send(uint8_t xbee, uint8_t frame, __code const char *command, __data const void *value, uint8_t val_length) {
	uint8_t header[4];
	xbee_txpacket_iovec_t txiovs[2];
	xbee_txpacket_t txpkt;

	header[0] = 0x08;
	header[1] = frame;
	header[2] = command[0];
	header[3] = command[1];

	txiovs[0].ptr = header;
	txiovs[0].len = sizeof(header);
	txiovs[1].ptr = value;
	txiovs[1].len = val_length;

	txpkt.num_iovs = 2;
	txpkt.iovs = txiovs;

	xbee_txpacket_queue(&txpkt, xbee);

	while (xbee_txpacket_dequeue() != &txpkt);
}

/**
 * \brief Issues an AT command to an XBee to get or set a parameter.
 *
 * \param[in] xbee the index of the XBee to send the command to.
 *
 * \param[in] frame the frame number to use.
 *
 * \param[in] command the AT command to send.
 *
 * \param[in] value the value to set the parameter to.
 *
 * \param[in] val_length the number of bytes to use to represent \p value.
 *
 * \param[out] resp the location to store the returned value in.
 *
 * \param[in] resp_length the size of response to expect.
 *
 * \return \c true on success, or \c false on failure.
 */
static BOOL at_command(uint8_t xbee, uint8_t frame, __code const char *command, const void *value, uint8_t val_length, __data void *resp, uint8_t resp_length) {
	uint8_t retries;
	__data xbee_rxpacket_t *rxpkt;
	BOOL success;

	for (retries = 0; retries != 6; ++retries) {
		/* Send the command. */
		at_send(xbee, frame, command, value, val_length);

		/* Wait for a response for up to a timeout. */
		TMR0H = 0;
		TMR0L = 0;
		INTCONbits.TMR0IF = 0;
		while (!INTCONbits.TMR0IF) {
			if ((rxpkt = xbee_rxpacket_get())) {
				/* See which XBee this packet came from. */
				if (rxpkt->xbee == xbee) {
					/* We received a packet. See what it is. */
					if (rxpkt->len == 5 + resp_length && rxpkt->buf[0] == 0x88 && rxpkt->buf[1] == frame) {
						/* It's an AT command response whose frame ID matches ours. */
						success = false;
						if ((char) rxpkt->buf[2] != command[0] || (char) rxpkt->buf[3] != command[1]) {
							/* The command is wrong. */
							error_reporting_add(FAULT_XBEE0_AT_RESPONSE_WRONG_COMMAND + xbee);
						} else if (rxpkt->buf[4] != 0) {
							/* The response is a failure. */
							switch (rxpkt->buf[4]) {
								case 2:
									error_reporting_add(FAULT_XBEE0_AT_RESPONSE_FAILED_INVALID_COMMAND + xbee);
									break;

								case 3:
									error_reporting_add(FAULT_XBEE0_AT_RESPONSE_FAILED_INVALID_PARAMETER + xbee);
									break;

								default:
									error_reporting_add(FAULT_XBEE0_AT_RESPONSE_FAILED_UNKNOWN_REASON + xbee);
									break;
							}
						} else {
							/* The command succeeded. */
							success = true;
							memcpyram2ram(resp, rxpkt->buf + 5, resp_length);
						}
						xbee_rxpacket_free(rxpkt);
						return success;
					} else {
						/* It's not a response to our request. Keep waiting. */
						xbee_rxpacket_free(rxpkt);
					}
				} else {
					/* This packet came from the wrong XBee. Ignore it. */
					xbee_rxpacket_free(rxpkt);
				}
			}
		}
	}

	/* Out of retries. Give up. */
	error_reporting_add(FAULT_XBEE0_TIMEOUT + xbee);
	return false;
}

/**
 * \brief Stage-1 configures an XBee.
 *
 * \param[in] xbee the index of the XBee to configure.
 *
 * \return \c true on success, or \c false on failure.
 */
static BOOL configure_xbee_stage1(uint8_t xbee) {
	uint8_t buffer[2];
	uint8_t err = 0;

	/* Enable RTS flow control on pin DIO6. */
	buffer[0] = 1;
	if (!at_command(xbee, 0x82, "D6", buffer, 1, 0, 0)) {
		err = FAULT_XBEE0_ENABLE_RTS_FAILED + xbee;
		goto out;
	}

	/* Retrieve the firmware version. */
	if (!at_command(xbee, 0x83, "VR", 0, 0, buffer, 2)) {
		err = FAULT_XBEE0_GET_FW_VERSION_FAILED + xbee;
		goto out;
	}
	xbee_versions[xbee] = (buffer[0] << 8) | buffer[1];

	/* Set up the text node ID. */
	if (!at_command(xbee, 0x85, "NI", (xbee == 0) ? "TBOTS30" : "TBOTS31", 7, 0, 0)) {
		err = FAULT_XBEE0_SET_NODE_ID_FAILED + xbee;
		goto out;
	}

out:
	/* Mark final status. */
	if (err) {
		error_reporting_add(err);
		return false;
	} else {
		return true;
	}
}

/**
 * \brief Stage-2 configures an XBee.
 *
 * \param[in] xbee the index of the XBee to configure.
 *
 * \return \c true on success, or \c false on failure.
 */
static BOOL configure_xbee_stage2(uint8_t xbee) {
	uint8_t buffer[2];
	uint8_t err = 0;

	/* Set the radio channel. */
	if (!at_command(xbee, 0x90, "CH", &params.xbee_channels[xbee], 1, 0, 0)) {
		err = FAULT_XBEE0_SET_CHANNEL_FAILED + xbee;
		goto out;
	}

	/* Set up the PAN ID. */
	buffer[0] = 0x49;
	buffer[1] = 0x6C + xbee;
	if (!at_command(xbee, 0x84, "ID", buffer, 2, 0, 0)) {
		err = FAULT_XBEE0_SET_PAN_ID_FAILED + xbee;
		goto out;
	}

	/* Set the 16-bit address. */
	buffer[0] = 0x7B;
	buffer[1] = (xbee == 0 ? 0x20 : 0x30) | params.robot_number;
	if (!at_command(xbee, 0x91, "MY", buffer, 2, 0, 0)) {
		err = FAULT_XBEE0_SET_ADDRESS_FAILED + xbee;
		goto out;
	}

out:
	/* Mark final status. */
	if (err) {
		error_reporting_add(err);
		return false;
	} else {
		return true;
	}
}

void main(void) {
	/* Configure I/O pins. */
	PINS_INITIALIZE();
	WDTCONbits.ADSHR = 1;
	ANCON0 = 0xF0;
	ANCON1 = 0xFF;
	WDTCONbits.ADSHR = 0;

	/* Disable timer 0 until needed. */
	T0CONbits.TMR0ON = 0;

	/* We want to attach timer 1 to ECCP 1 and timer 3 to ECCP2 so we can have separate periods. */
	T3CONbits.T3CCP2 = 0;
	T3CONbits.T3CCP1 = 1;

	/* Wait for the oscillator to start. */
	while (!OSCCONbits.OSTS);

	/* Enable the PLL and wait for it to lock. This may take up to 2ms. */
	OSCTUNEbits.PLLEN = 1;
	delay1ktcy(2);

	/* Configure interrupt handling. */
	RCONbits.IPEN = 1;
	INTCONbits.GIEH = 1;
	INTCONbits.GIEL = 1;

	/* Clear the feedback block. */
	memset(&feedback_block, 0, sizeof(feedback_block));

	/* Clear the drive block. */
	memset(&drive_block, 0, sizeof(drive_block));

	/* Configure timer 0 to a period of 1 / 12000000 × 65536 × 256 =~ 1.4s to be used as a serial line timeout for XBees.
	 *        /-------- Timer on
	 *        |/------- 16-bit timer
	 *        ||/------ Internal instruction clock
	 *        |||/----- Ignored
	 *        ||||/---- Prescaler active
	 *        |||||///- 1:256 prescale */
	T0CON = 0b10000111;

	/* Configure the SPI transceiver. */
	spi_init();

	/* Load the operational parameters block. */
	if (!params_load() || !params.xbee_channels[0] || !params.xbee_channels[1] || !params.robot_number) {
		/* Parameters corrupt or uninitialized.
		 * We can't do anything useful because our only communication mechanism is XBee and we don't know what channel or ID number to take. */
		for (;;) {
			Sleep();
		}
	}

	/* Start configuring the FPGA. */
	spi_tristate();
	LAT_FPGA_PROG_B = 1;

	/* Checksum the PIC's on-board ROM. */
	checksum_pic_rom();

	/* Configure the XBees. */
	LAT_XBEE0_RESET = 1;
	LAT_XBEE0_SLEEP = 0;
	LAT_XBEE1_RESET = 1;
	LAT_XBEE1_SLEEP = 0;
	serial_init();
	xbee_txpacket_init();
	xbee_rxpacket_init();
	LAT_LED2 = 1;
	if (!configure_xbee_stage1(0)) {
		LAT_FPGA_PROG_B = 0;
		for (;;) {
			Sleep();
		}
	}
	if (!configure_xbee_stage2(0)) {
		LAT_FPGA_PROG_B = 0;
		for (;;) {
			Sleep();
		}
	}
	LAT_LED2 = 0;
	LAT_LED3 = 1;
	if (!configure_xbee_stage1(1)) {
		LAT_FPGA_PROG_B = 0;
		for (;;) {
			Sleep();
		}
	}
	if (!configure_xbee_stage2(1)) {
		LAT_FPGA_PROG_B = 0;
		for (;;) {
			Sleep();
		}
	}
	LAT_LED1 = 1;
	LAT_LED3 = 0;

	/* Wait for FPGA configuration to finish. */
	while (!PORT_FPGA_INIT_B);
	while (!PORT_FPGA_DONE && PORT_FPGA_INIT_B);
	if (!PORT_FPGA_DONE) {
		error_reporting_add(FAULT_FPGA_INVALID_BITSTREAM);
		LAT_FPGA_PROG_B = 0;
	} else {
		delay1ktcy(1);
		LAT_DCM_RESET = 0;
		delay1ktcy(1);
		while (!PORT_DCM_LOCKED);
		delay1ktcy(1);
		LAT_FPGA_RESET = 0;

		/* At startup the following sequence of events will occur:
		 * 1. A 16-bit shift register containing all ones will begin shifting out the Reset signal on the 1MHz clock.
		 *    The shift register will hence release the internal GSR net after 16 / 1e6 = 16µs.
		 * 2. The FlashChecksummer will count off a total of three SPI clock ticks at 32Mhz, for a total of 3 / 32e6 = 93.75ns.
		 * 3. The FlashChecksummer will assert /CS and start checksumming the Flash, which will take around half a second.
		 *
		 * The total time before /CS is asserted is 16µs + 93.75ns = 16.09375µs.
		 * We need to wait until the checksumming operation is finished before reasserting control of the SPI bus.
		 * We can detect the beginning of the checksumming operation by waiting for (with a generous margin) 64µs × 12MIPS = 768 instruction cycles. */
		delay100tcy(8);

		/* We can then detect the end of the checksumming operation by waiting for /CS to be deasserted. */
		while (!PORT_FLASH_CS);
	}
	spi_drive();

	/* Configure the parallel master port.
	 *         /-------- Module disabled
	 *         |/------- Unimplemented
	 *         ||/------ Continue operation in idle mode
	 *         |||//---- Address and data on separate pins
	 *         |||||/--- Byte enable pin disabled
	 *         ||||||/-- Write strobe pin enabled
	 *         |||||||/- Read strobe pin enabled */
	PMCONH = 0b00000011;
	/*         //------- No chip select pins
	 *         ||////--- Ignored (polarity control for unused pins)
	 *         ||||||/-- Write strobe active high
	 *         |||||||/- Read strobe active high */
	PMCONL = 0b00000011;
	/*          /-------- Port not busy
	 *          |//------ No interrupts used
	 *          |||//---- No address increment
	 *          |||||/--- 8-bit data width
	 *          ||||||//- Master mode 2 (separate read and write strobes) */
	PMMODEH = 0b00000010;
	/*          //------- Ignored
	 *          ||////--- No wait cycles, entire bus operation occurs in one cycle
	 *          ||||||//- Ignored */
	PMMODEL = 0b00000000;
	/*       /-------- PMA15 disabled, functions as port I/O
	 *       |/------- PMA14 disabled, functions as port I/O
	 *       ||/------ PMA13 disabled, functions as port I/O
	 *       |||/----- PMA12 disabled, functions as port I/O
	 *       ||||/---- PMA11 disabled, functions as port I/O
	 *       |||||/--- PMA10 disabled, functions as port I/O
	 *       ||||||/-- PMA9 disabled, functions as port I/O
	 *       |||||||/- PMA8 disabled, functions as port I/O */
	PMEH = 0b00000000;
	/*       /-------- PMA7 disabled, functions as port I/O
	 *       |/------- PMA6 disabled, functions as port I/O
	 *       ||/------ PMA5 disabled, functions as port I/O
	 *       |||/----- PMA4 disabled, functions as port I/O
	 *       ||||/---- PMA3 disabled, functions as port I/O
	 *       |||||/--- PMA2 disabled, functions as port I/O
	 *       ||||||/-- PMA1 disabled, functions as port I/O
	 *       |||||||/- PMA0 disabled, functions as port I/O */
	PMEL = 0b00000000;
	/* Enable the module. */
	PMCONHbits.PMPEN = 1;

	/* Run the main loop. */
	run();
}

