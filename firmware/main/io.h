#ifndef IO_H
#define IO_H

/**
 * \file
 *
 * \brief Provides access to hardware I/O ports
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

#include <stdint.h>

#define IO_PORT(x) (*(volatile uint8_t *) ((x) + 32))

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
#define LED_CTL IO_PORT(0x00)

/**
 * \brief Controls power to various parts of the robot
 *
 * Bits:
 * 7–5: Reserved
 * 4 (R): Breakout board present; 1 = present, 0 = absent
 * 3 (R/W) [0]: Laser power; 1 = lit, 0 = unlit
 * 2 (R): Interlocks overridden; 1 = overridden, 0 = not overridden
 * 1 (R/W) [0]: Motor power; 1 = run, 0 = power down
 * 0 (R/W) [1]: Logic power; 1 = run, 0 = power down
 */
#define POWER_CTL IO_PORT(0x01)

/**
 * \brief A real-time timer counting 5 millisecond ticks
 *
 * Bits:
 * 7–0 (R) [00000000]: Count of ticks since power-up
 */
#define TICKS IO_PORT(0x02)

/**
 * \brief Selects which motor will be controlled.
 *
 * The value in this register controls which registers are viewed by the MOTOR_CTL, MOTOR_STATUS, and MOTOR_PWM locations.
 *
 * Bits:
 * 7–0 (R/W) [00000000]: Motor index
 *
 * Values:
 * 0: Motor registers control wheel 0
 * 1: Motor registers control wheel 1
 * 2: Motor registers control wheel 2
 * 3: Motor registers control wheel 3
 * 4: Motor registers control dribbler
 */
#define MOTOR_INDEX IO_PORT(0x03)

/**
 * \brief Controls the overall operation of the motor.
 *
 * Bits:
 * 7–6 (R/W) [00]: Phase 2 control
 * 5–4 (R/W) [00]: Phase 1 control
 * 3–2 (R/W) [00]: Phase 0 control
 * 1 (R/W) [0]: Automatic commutation
 * 0 (R/W) [0]: Direction
 *
 * When automatic commutation is enabled, the Direction bit controls the direction of motor power and the Phase bits are ignored.
 * When automatic commutation is disabled, the Direction bit is ignored and the Phase bits control the motor phases directly.
 *
 * Direction values:
 * 0: Forward
 * 1: Reverse
 *
 * Phase values:
 * 00: Float
 * 01: PWM
 * 10: Low
 * 11: High
 *
 * Unless interlocks are overridden, the upper six bits can only be 000000 or 101010.
 * Any write that does not satisfy this constraint will result in these bits being set to 000000.
 * The same will happen if interlocks are re-enabled while an illegal value is set.
 */
#define MOTOR_CTL IO_PORT(0x04)

/**
 * \brief Reports the status of the motor.
 *
 * Bits:
 * 7–2: Reserved
 * 1 (R/C) [0]: Hall sensors observed all high; 1 = failure observed, 0 = failure not observed
 * 0 (R/C) [0]: Hall sensors observed all low; 1 = failure observed, 0 = failure not observed
 */
#define MOTOR_STATUS IO_PORT(0x05)

/**
 * \brief The PWM duty cycle for the motor.
 *
 * Bits:
 * 7–0 (R/W) [00]: Duty cycle
 */
#define MOTOR_PWM IO_PORT(0x06)

/**
 * \brief Used in simulation only to move magic values from firmware to testbench
 *
 * A write sets the magic value.
 * A read returns the most recently written value.
 */
#define SIM_MAGIC IO_PORT(0x07)

/**
 * \brief Controls and reports status of the Secure Digital card
 *
 * Bits:
 * 7–6: Reserved
 * 5 (R) [0]: Indicates whether a past DMA-driven multiblock write encountered a malformed data response token from the card.
 * 4 (R) [0]: Indicates whether a past DMA-driven multiblock write encountered a write error in a data response token from the card.
 * 3 (R) [0]: Indicates whether a past DMA-driven multiblock write encountered a CRC error in a data response token from the card.
 * 2 (R/W) [1]: Sets the level on the /CS pin
 * 1 (R): Indicates whether a card is present according to the card detection switch; 1 = present, 0 = absent
 * 0 (R) [0]: Indicates whether an SPI transaction is in progress; 1 = busy, 0 = idle
 */
#define SD_CTL IO_PORT(0x08)

/**
 * \brief Reads and writes data on the Secure Digital SPI bus
 *
 * On write, starts an SPI transaction outputting the written byte
 * On read (when SD_CTL<0> = 0), returns the most recent byte read from the bus
 */
#define SD_DATA IO_PORT(0x09)

/**
 * \brief Reports the accumulated count of optical encoder ticks
 *
 * A write to this register selects an optical encoder (0–3) and simultaneously snapshots and clears the accumulated count.
 * A read from this register returns the LSB of the snapshot value.
 */
#define ENCODER_LSB IO_PORT(0x0A)

/**
 * \brief Reports the accumulated count of optical encoder ticks
 *
 * A read from this register reports the MSB of the snapshot value taken by a write to \ref ENCODER_LSB.
 */
#define ENCODER_MSB IO_PORT(0x0B)

/**
 * \brief Reports encoder commutation failures
 *
 * Bits:
 * 7–4: Reserved
 * 3 (R) [0]: Encoder 3 not commutating during wheel rotation; 1 = failure observed, 0 = failure not observed
 * 2 (R) [0]: Encoder 2 not commutating during wheel rotation; 1 = failure observed, 0 = failure not observed
 * 1 (R) [0]: Encoder 1 not commutating during wheel rotation; 1 = failure observed, 0 = failure not observed
 * 0 (R) [0]: Encoder 0 not commutating during wheel rotation; 1 = failure observed, 0 = failure not observed
 */
#define ENCODER_FAIL IO_PORT(0x0C)

/**
 * \brief Reports mainboard analogue-to-digital converter readings
 *
 * A write to this register selects a channel (0–7) and snapshots the most recent conversion result for the channel.
 * A read from this register returns the LSB of the snapshot value.
 */
#define ADC_LSB IO_PORT(0x0D)

/**
 * \brief Reports mainboard analogue-to-digital converter readings
 *
 * A read from this register reports the MSB of the snapshot value taken by a write to \ref ADC_LSB.
 */
#define ADC_MSB IO_PORT(0x0E)

/**
 * \brief Controls and reports on the chicker subsystem
 *
 * Bits:
 * 7: Reserved
 * 6 (R): Indicates whether the chicker board is present; 1 = present, 0 = absent
 * 5 (R/W) [0]: Activates the safe discharge circuit to discharge the capacitors; 1 = enabled, 0 = disabled
 * 4 (R) [0]: Indicates whether the capacitors are fully charged; 1 = charged, 0 = not charged
 * 3 (R) [0]: Indicates whether charging timed out; 1 = timeout detected, 0 = timeout not detected
 * 2 (R/S) [0]: Fires the chipper; 1 = fire, 0 = do not fire, cleared in hardware at end of pulse
 * 1 (R/S) [0]: Fires the kicker; 1 = fire, 0 = do not fire, cleared in hardware at end of pulse
 * 0 (R/W) [0]: Enables the charger; 1 = charge, 0 = do not charge
 */
#define CHICKER_CTL IO_PORT(0x0F)

/**
 * \brief Sets the chicker firing pulse width
 *
 * The actual width of the generated pulse is (N + 1) ÷ 4 microseconds, where N is the value loaded into the registers.
 *
 * Bits:
 * 7–0 (R/W) [00000000]: LSB of pulse width setting
 */
#define CHICKER_PULSE_LSB IO_PORT(0x10)

/**
 * \brief Sets the chicker firing pulse width
 *
 * Bits:
 * 7–0 (R/W) [00000000]: MSB of pulse width setting
 */
#define CHICKER_PULSE_MSB IO_PORT(0x11)

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
#define FLASH_CTL IO_PORT(0x12)

/**
 * \brief Reads and writes data on the SPI Flash bus
 *
 * A transaction must not be started unless the debug port is disabled.
 *
 * On write, starts an SPI transaction outputting the written byte
 * On read (when FLASH_CTL<0> = 0), returns the most recent byte read from the bus
 */
#define FLASH_DATA IO_PORT(0x13)

/**
 * \brief Controls and reports status of the MRF24J40
 *
 * Bits:
 * 7 (R): Indicates whether an operation is currently executing; 1 = busy, 0 = idle
 * 6 (S): Starts a long-address register write on set
 * 5 (S): Starts a short-address register write on set
 * 4 (S): Starts a long-address register read on set
 * 3 (S): Starts a short-address register read on set
 * 2 (R): Reports the level on the INT pin
 * 1 (R/W) [0]: Sets the level on the WAKE pin
 * 0 (R/W) [1]: Sets the level on the /RESET pin
 */
#define MRF_CTL IO_PORT(0x14)

/**
 * \brief Provides access to the register value.
 *
 * On write, sets the value that will be written in a register write.
 * On read, returns the value most recently read by a register read.
 */
#define MRF_DATA IO_PORT(0x15)

/**
 * \brief Sets the address of the register to be accessed in the MRF24J40.
 *
 * On write, transfers the previous written byte to the MSB of the address and writes the new value to the LSB.
 * On read, returns an unspecified value.
 */
#define MRF_ADDR IO_PORT(0x16)

/**
 * \brief Controls the lateral position sensor
 *
 * Bits:
 * 7–4: Reserved
 * 3–0 (R/W) [0]: Drive LPS LEDs; 1 = LED lit, 0 = LED dark
 */
#define LPS_CTL IO_PORT(0x17)

/**
 * \brief Holds the least significant byte of the device DNA.
 *
 * Bits:
 * 7–0 (R): The byte
 */
#define DEVICE_ID0 IO_PORT(0x18)
#define DEVICE_ID1 IO_PORT(0x19)
#define DEVICE_ID2 IO_PORT(0x1A)
#define DEVICE_ID3 IO_PORT(0x1B)
#define DEVICE_ID4 IO_PORT(0x1C)
#define DEVICE_ID5 IO_PORT(0x1D)
#define DEVICE_ID6 IO_PORT(0x1E)

/**
 * \brief Status of the device dna registers
 *
 * 7-1: Reserved
 * 0 (R) [0]: Valid ID; 1 = Device DNA is valid, 0 = Device DNA not ready
 */
#define DEVICE_ID_STATUS IO_PORT(0x1F)

/**
 * \brief Linear feedback shift register control register
 *
 * 7-1: Reserved
 * 0 (R/W) [1]: A write produces an increment of the LFSR, a read provides the LSb of its current value
 */
#define LFSR IO_PORT(0x20)

/**
 * \brief Controls and reports the status of the debug port
 *
 * The debug port must not be enabled unless the flash memory SPI port is idle and chip select is deasserted.
 *
 * 7–2: Reserved
 * 1 (R) [0]: Indicates whether a debug port transaction is in progress; 1 = busy, 0 = idle
 * 0 (R/W) [0]: Controls whether the debug port is enabled; 1 = enabled, 0 = idle
 */
#define DEBUG_CTL IO_PORT(0x21)

/**
 * \brief Starts a transaction on the debug port
 *
 * A write to this register sends the written value over the debug port.
 *
 * The debug port must be enabled before starting a transaction.
 * Writes to this register while the port is either disabled or busy will be discarded.
 */
#define DEBUG_DATA IO_PORT(0x22)

/**
 * \brief Controls and reports the status of the internal configuration access port
 *
 * 7–1: Reserved
 * 0 (R) [0]: Indicates whether the ICAP is busy; 1 = busy, 0 = idle
 */
#define ICAP_CTL IO_PORT(0x23)

/**
 * \brief Starts a transaction on the FPGA’s internal configuration access port
 *
 * A write to this register initiates a transaction using the written value along with the value most recently written to \ref ICAP_MSB.
 * A read returns the most recently written value.
 */
#define ICAP_LSB IO_PORT(0x24)

/**
 * \brief Buffers the MSB of the next ICAP transaction
 *
 * A write sets the buffer value.
 * A read returns the most recently written value.
 */
#define ICAP_MSB IO_PORT(0x25)

/**
 * \brief Holds the channel number of the DMA channel whose control registers are currently visible on the other DMA ports
 *
 * This register has an unspecified value at startup.
 * A write to this register exposes a new set of control registers.
 * A read returns the most recently written value.
 */
#define DMA_CHANNEL IO_PORT(0x26)

/**
 * \brief Sets the address of the next byte to be read or written by the selected DMA channel.
 *
 * As a memory address is 16 bits wide, it is necessary to write to this register twice to set the complete address.
 * The first write operation sets the MSB of the address and the second sets the LSB.
 * Reading the register returns an unspecified value.
 *
 * This register must not be written to when the channel is enabled.
 *
 * Addresses used by the DMA controller must lie only within data memory and are therefore 96 bytes smaller than the same address used in CPU code.
 */
#define DMA_PTR IO_PORT(0x27)

/**
 * \brief Sets the length of the next transfer on the selected DMA channel.
 *
 * As DMA transfers can be more than 256 byte slong, it is necessary to write to this register twice to set the complete byte count.
 * The first write operation sets the MSB of the count and the second sets the LSB.
 * Reading the register returns an unspecified value.
 *
 * This register must not be written to when the channel is enabled.
 */
#define DMA_COUNT IO_PORT(0x28)

/**
 * \brief Holds the enable bit of the selected DMA channel
 *
 * 7–1: Reserved
 * 0 [R/S] (0): Indicates whether the DMA channel is enabled; 1 = enabled, 0 = disabled
 *
 * To execute a DMA block transfer on a channel, the software must first, with the channel disabled, ensure that \ref DMA_PTRL, \ref DMA_PTRH, and \ref DMA_COUNT are set properly.
 * Software must also ensure the peripheral is ready to execute a DMA transfer.
 * Software must then write a 1 to the enable bit in this register to start the transfer.
 * Once the transfer is enabled, software must not write to the \ref DMA_PTRL, \ref DMA_PTRH, or \ref DMA_COUNT registers, nor rely on a value read from them.
 * Once the transfer completes, hardware clears the enable bit automatically.
 * When this happens, \ref DMA_PTRL and \ref DMA_PTRH point to the first byte after the transferred block, and \ref DMA_COUNT is zero.
 * Completion of the DMA transfer may or may not imply completion of the peripheral activities related to the transfer; this is peripheral-specific.
 */
#define DMA_CTL IO_PORT(0x29)

/**
 * \brief Reports break beam voltage difference
 * 
 * A write to this register records the voltage difference
 * A read from this register returns the LSB of the snapshot value
 */
#define BREAKBEAM_DIFF_L IO_PORT(0x2A)

/**
 * \brief Reports break beam voltage difference
 * 
 * A read from this register returns the MSB of the snapshot value
 */
#define BREAKBEAM_DIFF_H IO_PORT(0x2B)

/**
 * \brief The LSB of the stack pointer
 */
#define SP_LSB IO_PORT(0x3D)

/**
 * \brief The MSB of the stack pointer
 */
#define SP_MSB IO_PORT(0x3E)

/**
 * \brief The status register
 */
#define SREG IO_PORT(0x3F)

#endif

