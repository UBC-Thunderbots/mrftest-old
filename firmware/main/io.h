#ifndef IO_H
#define IO_H

/**
 * \file
 *
 * \brief Provides access to hardware I/O ports
 */

#include <stdint.h>

/**
 * \brief Reads from an I/O port
 *
 * \param[in] port the port to read from
 *
 * \return the port’s current value
 */
static uint8_t inb(uint8_t port) __attribute__((unused));

/**
 * \brief Writes to an I/O port
 *
 * \param[in] port the port to write to
 *
 * \param[in] value the value to write
 */
static void outb(uint8_t port, uint8_t value) __attribute__((unused));

/**
 * \brief The I/O ports
 *
 * For each port, the purpose of each bit is defined.
 * Reserved bits should be maintained at zero.
 * The value in square brackets is the power-up default value.
 *
 * Legend:
 * R - bit is readable
 * W - bit is generally writeable
 * S - bit is writeable to 1 in software; writing 0 has no effect
 * C - bit is writeable to 0 in software; writing 1 has no effect
 */
typedef enum {
	/**
	 * \brief Controls the operation of the LEDs on the mainboard
	 *
	 * Bits:
	 * 7 (R/W) [0]: Radio LED control; 1 = on, 0 = off
	 * 6 (R/S) [0]: Radio LED blink; 1 = blink, 0 = no blink; cleared by hardware
	 * 5 (R/W) [1]: Test LED signal source; 1 = direct software control, 0 = hardware source
	 * 4–0 [00000]: (R/W): Test LED value:
	 *   If bit 5 = 1:
	 *     4–3: Reserved
	 *     2–0: Test LED values, 1 = LED on, 0 = LED off
	 *   If bit 5 = 0:
	 *     00000 = LEDs display Hall sensors for wheel 0
	 *     00001 = LEDs display Hall sensors for wheel 1
	 *     00010 = LEDs display Hall sensors for wheel 2
	 *     00011 = LEDs display Hall sensors for wheel 3
	 *     00100 = LEDs display Hall sensors for dribbler
	 *     00101 = LEDs display optical encoder for wheel 0
	 *     00110 = LEDs display optical encoder for wheel 1
	 *     00111 = LEDs display optical encoder for wheel 2
	 *     01000 = LEDs display optical encoder for wheel 3
	 *     Others: Reserved
	 */
	LED_CTL = 0x00,

	/**
	 * \brief Controls power to various parts of the robot
	 *
	 * Bits:
	 * 7–3: Reserved
	 * 2 (R/W) [0]: Chicker power; 1 = run, 0 = power down
	 * 1 (R/W) [0]: Motor power; 1 = run, 0 = power down
	 * 0 (R/W) [1]: Logic power; 1 = run, 0 = power down
	 */
	POWER_CTL = 0x01,

	/**
	 * \brief A real-time timer counting 5 millisecond ticks
	 *
	 * Bits:
	 * 7–0 (R) [00000000]: Count of ticks since power-up
	 */
	TICKS = 0x02,

	/**
	 * \brief Controls wheel motors between floating, braking, and driving in one of two directions
	 *
	 * Bits:
	 * 7–6 (R/W) [00]: Motor 3 control
	 * 5–4 (R/W) [00]: Motor 2 control
	 * 3–2 (R/W) [00]: Motor 1 control
	 * 1–0 (R/W) [00]: Motor 0 control
	 *
	 * Values:
	 * 00 = float
	 * 01 = brake
	 * 10 = drive forward
	 * 11 = drive backward
	 */
	WHEEL_CTL = 0x03,

	/**
	 * \brief Reports Hall sensor failures on wheel motors
	 *
	 * Bits:
	 * 7 (R/C) [0]: Wheel 3 Halls observed all high; 1 = failure observed, 0 = failure not observed
	 * 6 (R/C) [0]: Wheel 2 Halls observed all high; 1 = failure observed, 0 = failure not observed
	 * 5 (R/C) [0]: Wheel 1 Halls observed all high; 1 = failure observed, 0 = failure not observed
	 * 4 (R/C) [0]: Wheel 0 Halls observed all high; 1 = failure observed, 0 = failure not observed
	 * 3 (R/C) [0]: Wheel 3 Halls observed all low; 1 = failure observed, 0 = failure not observed
	 * 2 (R/C) [0]: Wheel 2 Halls observed all low; 1 = failure observed, 0 = failure not observed
	 * 1 (R/C) [0]: Wheel 1 Halls observed all low; 1 = failure observed, 0 = failure not observed
	 * 0 (R/C) [0]: Wheel 0 Halls observed all low; 1 = failure observed, 0 = failure not observed
	 */
	WHEEL_HALL_FAIL = 0x04,

	/**
	 * \brief PWM duty cycle for motor 0 drive
	 */
	WHEEL0_PWM = 0x05,

	/**
	 * \brief PWM duty cycle for motor 1 drive
	 */
	WHEEL1_PWM = 0x06,

	/**
	 * \brief PWM duty cycle for motor 2 drive
	 */
	WHEEL2_PWM = 0x07,

	/**
	 * \brief PWM duty cycle for motor 3 drive
	 */
	WHEEL3_PWM = 0x08,

	/**
	 * \brief Controls the dribbler motor’s operating mode
	 *
	 * Bits:
	 * 7–2: Reserved
	 * 1–0 (R/W) [00]: Dribbler motor control
	 *
	 * Values:
	 * 00 = float
	 * 01 = brake
	 * 10 = drive forward
	 * 11 = drive backward
	 */
	DRIBBLER_CTL = 0x09,

	/**
	 * \brief Reports Hall sensor failures on the dribbler motor
	 *
	 * Bits:
	 * 7–2: Reserved
	 * 1 (R/C) [0]: Dribbler Halls observed all high; 1 = failure observed, 0 = failure not observed
	 * 0 (R/C) [0]: Dribbler Halls observed all low; 1 = failure observed, 0 = failure not observed
	 */
	DRIBBLER_HALL_FAIL = 0x0A,

	/**
	 * \brief PWM duty cycle for the dribbler motor
	 */
	DRIBBLER_PWM = 0x0B,

	/**
	 * \brief Reports the accumulated count of optical encoder ticks
	 *
	 * A write to this register selects an optical encoder (0–3) and simultaneously snapshots and clears the accumulated count.
	 * A read from this register returns the LSB of the snapshot value.
	 */
	ENCODER_LSB = 0x0C,

	/**
	 * \brief Reports the accumulated count of optical encoder ticks
	 *
	 * A read from this register reports the MSB of the snapshot value taken by a write to \ref ENCODER_LSB.
	 */
	ENCODER_MSB = 0x0D,

	/**
	 * \brief Reports encoder commutation failures
	 *
	 * Bits:
	 * 7–4: Reserved
	 * 3 (R/C) [0]: Encoder 3 not commutating during wheel rotation; 1 = failure observed, 0 = failure not observed
	 * 2 (R/C) [0]: Encoder 2 not commutating during wheel rotation; 1 = failure observed, 0 = failure not observed
	 * 1 (R/C) [0]: Encoder 1 not commutating during wheel rotation; 1 = failure observed, 0 = failure not observed
	 * 0 (R/C) [0]: Encoder 0 not commutating during wheel rotation; 1 = failure observed, 0 = failure not observed
	 */
	ENCODER_FAIL = 0x0E,

	/**
	 * \brief Reports mainboard analogue-to-digital converter readings
	 *
	 * A write to this register selects a channel (0–3) and snapshots the most recent conversion result for the channel.
	 * A read from this register returns the LSB of the snapshot value.
	 */
	ADC_LSB = 0x0F,

	/**
	 * \brief Reports mainboard analogue-to-digital converter readings
	 *
	 * A read from this register reports the MSB of the snapshot value taken by a write to \ref ADC_LSB.
	 */
	ADC_MSB = 0x10,

	/**
	 * \brief Reports chicker analogue-to-digital converter readings
	 *
	 * A write to this register snapshots the most recent conversion result.
	 * A read from this register returns the LSB of the snapshot value.
	 */
	CHICKER_ADC_LSB = 0x11,

	/**
	 * \brief Reports chicker analogue-to-digital converter readings
	 *
	 * A read from this register returns the MSB of the snapshot value taken by a write to \ref CHICKER_ADC_LSB.
	 */
	CHICKER_ADC_MSB = 0x12,

	/**
	 * \brief Controls and reports on the chicker subsystem
	 *
	 * Bits:
	 * 7–6: Reserved
	 * 5 (R/W) [0]: Activates the safe discharge circuit to discharge the capacitors; 1 = enabled, 0 = disabled
	 * 4 (R) [0]: Indicates whether the capacitors are fully charged; 1 = charged, 0 = not charged
	 * 3 (R) [0]: Indicates whether charging timed out; 1 = timeout detected, 0 = timeout not detected
	 * 2 (R/S) [0]: Fires the chipper; 1 = fire, 0 = do not fire, cleared in hardware at end of pulse
	 * 1 (R/S) [0]: Fires the kicker; 1 = fire, 0 = do not fire, cleared in hardware at end of pulse
	 * 0 (R/W) [0]: Enables the charger; 1 = charge, 0 = do not charge
	 */
	CHICKER_CTL = 0x13,

	/**
	 * \brief Sets the chicker firing pulse width
	 *
	 * The actual width of the generated pulse is (N + 1) ÷ 4 microseconds, where N is the value loaded into the registers.
	 *
	 * Bits:
	 * 7–0 (R/W) [00000000]: LSB of pulse width setting
	 */
	CHICKER_PULSE_LSB = 0x14,

	/**
	 * \brief Sets the chicker firing pulse width
	 *
	 * Bits:
	 * 7–0 (R/W) [00000000]: MSB of pulse width setting
	 */
	CHICKER_PULSE_MSB = 0x15,

	/**
	 * \brief Controls and reports status of the SPI Flash
	 *
	 * The chip select pin must not be asserted unless the debug port is disabled.
	 *
	 * Bits:
	 * 7–2: Reserved
	 * 1 (R/W) [1]: Sets the level on the /CS pin
	 * 0 (R) [0]: Indicates whether an SPI transaction is in progress; 1 = busy, 0 = idle
	 */
	FLASH_CTL = 0x16,

	/**
	 * \brief Reads and writes data on the SPI Flash bus
	 *
	 * A transaction must not be started unless the debug port is disabled.
	 *
	 * On write, starts an SPI transaction outputting the written byte
	 * On read (when FLASH_CTL<0> = 0), returns the most recent byte read from the bus
	 */
	FLASH_DATA = 0x17,

	/**
	 * \brief Controls and reports status of the MRF24J40
	 *
	 * Bits:
	 * 7–5: Reserved
	 * 4 (R): Reports the level on the INT pin
	 * 3 (R/W) [0]: Sets the level on the WAKE pin
	 * 2 (R/W) [1]: Sets the level on the /RESET pin
	 * 1 (R/W) [1]: Sets the level on the /CS pin
	 * 0 (R) [0]: Indicates whether an SPI transaction is in progress; 1 = busy, 0 = idle
	 */
	MRF_CTL = 0x18,

	/**
	 * \brief Reads and writes data on the MRF24J40 SPI bus
	 *
	 * On write, starts an SPI transaction outputting the written byte
	 * On read (when MRF_CTL<0> = 0), returns the most recent byte read from the bus
	 */
	MRF_DATA = 0x19,

	/**
	 * \brief Controls the break beam LED
	 *
	 * Bits:
	 * 7–1: Reserved
	 * 0 (R/W) [0]: LED drive; 1 = LED on, 0 = LED off
	 */
	BREAK_BEAM_CTL = 0x1A,

	/**
	 * \brief Controls the lateral position sensor
	 *
	 * Bits:
	 * 7–2: Reserved
	 * 1 (R/W) [1]: Reset LPS; 1 = hold in reset, 0 = release from reset
	 * 0 (R/W) [0]: Clock LPS; 1 = clock high, 0 = clock low
	 */
	LPS_CTL = 0x1B,

	DEVICE_ID0 = 0x1C,
	DEVICE_ID1 = 0x1D,
	DEVICE_ID2 = 0x1E,
	DEVICE_ID3 = 0x1F,
	DEVICE_ID4 = 0x20,
	DEVICE_ID5 = 0x21,
	DEVICE_ID6 = 0x22,
	DEVICE_ID_STATUS = 0x23,

	/**
	 * \brief Linear feedback shift register control register
	 *
	 * 7-1: Reserved
	 * 0 (R/W) [1]: A write produces an increment of the LFSR, a read provides the LSb of its current value
	 */
	LFSR = 0x24,

	/**
	 * \brief Controls and reports the status of the debug port
	 *
	 * The debug port must not be enabled unless the flash memory SPI port is idle and chip select is deasserted.
	 *
	 * 7–2: Reserved
	 * 1 (R) [0]: Indicates whether a debug port transaction is in progress; 1 = busy, 0 = idle
	 * 0 (R/W) [0]: Controls whether the debug port is enabled; 1 = enabled, 0 = idle
	 */
	DEBUG_CTL = 0x25,

	/**
	 * \brief Starts a transaction on the debug port
	 *
	 * A write to this register sends the written value over the debug port.
	 *
	 * The debug port must be enabled before starting a transaction.
	 * Writes to this register while the port is either disabled or busy will be discarded.
	 */
	DEBUG_DATA = 0x26,

	/**
	 * \brief The LSB of the stack pointer
	 */
	SP_LSB = 0x3D,

	/**
	 * \brief The MSB of the stack pointer
	 */
	SP_MSB = 0x3E,

	/**
	 * \brief The status register
	 */
	SREG = 0x3F,
} port_t;



static inline uint8_t inb(uint8_t port) {
	uint8_t value;
	asm volatile("in %0, %1" : "=r"(value) : "i"(port));
	return value;
}

static inline void outb(uint8_t port, uint8_t value) {
	asm volatile("out %0, %1" : : "i"(port), "r"(value));
}

#endif

