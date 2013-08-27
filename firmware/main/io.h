#ifndef IO_H
#define IO_H

/**
 * \file
 *
 * \brief Provides access to hardware I/O ports
 */



#include <stdint.h>



/**
 * \brief The base address of the AHB I/O area.
 */
#define IO_AHB_BASE 0xA0000000

/**
 * \brief The base address of the APB I/O area.
 */
#define IO_APB_BASE 0xC0000000



/**
 * \brief The type of the control and status register in the system control module.
 */
typedef struct {
	unsigned : 21;

	/**
	 * \brief Performs an AMBA reset when set.
	 *
	 * An AMBA reset reboots the CPU and resets all peripherals, but does not reload the FPGA configuration bitstream.
	 */
	unsigned amba_reset : 1;

	/**
	 * \brief Whether or not the device DNA is ready to read.
	 */
	unsigned device_dna_ready : 1;

	/**
	 * \brief The states of the test LEDs.
	 *
	 * Initialized to 000 at startup.
	 */
	unsigned test_leds : 3;

	/**
	 * \brief The state of the radio LED.
	 *
	 * Initialized to 0 at startup.
	 */
	unsigned radio_led : 1;

	/**
	 * \brief Whether or not the breakout board is present.
	 *
	 * Read-only.
	 */
	unsigned breakout_present : 1;

	/**
	 * \brief Whether or not safety interlocks are applied.
	 *
	 * Initialized to 1 at startup.
	 * Can only be set to 0 if hardware_interlock is 0.
	 */
	unsigned software_interlock : 1;

	/**
	 * \brief Whether or not the hardware interlock override jumper requires interlocks to be applied.
	 */
	unsigned hardware_interlock : 1;

	/**
	 * \brief Whether or not power is sent to the motor drivers.
	 *
	 * Initialized to 0 at startup.
	 */
	unsigned motor_power : 1;

	/**
	 * \brief Whether or not power is sent to the logic components.
	 *
	 * Initialized to 1 at startup.
	 */
	unsigned logic_power : 1;
} io_sysctl_csr_t;

/**
 * \brief The type of the registers in the system control module.
 */
typedef struct {
	/**
	 * \brief The system control and status register.
	 */
	io_sysctl_csr_t csr;

	/**
	 * \brief The upper 32 bits of the device DNA.
	 */
	const unsigned device_dna_high;

	/**
	 * \brief The lower 32 bits of the device DNA.
	 */
	const unsigned device_dna_low;

	/**
	 * \brief A counter of clock cycles passed since system boot.
	 */
	const unsigned tsc;

	/**
	 * \brief The battery voltage, as an ADC reading from 0 to 1023.
	 */
	const unsigned battery_voltage;

	/**
	 * \brief The thermistor voltage, as an ADC reading from 0 to 1023.
	 */
	const unsigned thermistor_voltage;

	/**
	 * \brief The laser difference, as an ADC reading difference in the range ±1023.
	 */
	const int laser_difference;
} io_sysctl_t;

/**
 * \brief The system control module.
 */
#define IO_SYSCTL (*(volatile io_sysctl_t *) (IO_APB_BASE + 0x00000))



/**
 * \brief The type of the debug serial port’s control and status register.
 */
typedef struct {
		const unsigned : 30;

		/**
		 * \brief Whether or not a transmission is currently occurring.
		 */
		const unsigned busy : 1;

		/**
		 * \brief Whether or not the port is enabled.
		 *
		 * Initialized to 0 at startup.
		 */
		unsigned enabled : 1;
} io_debug_port_csr_t;

/**
 * \brief The type of the registers controlling the debug serial port.
 */
typedef struct {
	/**
	 * \brief Reports the status of and controls the operation of the register.
	 */
	io_debug_port_csr_t csr;

	/**
	 * \brief Sends data to the serial port.
	 *
	 * Only the lower 8 bits of the written value are transmitted.
	 * This register should only be written when the port is not busy.
	 * This register should not be read.
	 */
	unsigned data;
} io_debug_port_t;

/**
 * \brief The debug serial port.
 */
#define IO_DEBUG_PORT (*(volatile io_debug_port_t *) (IO_APB_BASE + 0x00100))



/**
 * \brief The type of the MRF SPI port’s control and status register.
 */
typedef struct {
	unsigned : 24;

	/**
	 * \brief Whether or not the last DMA operation ended due to an error.
	 *
	 * Initialized to 0 at startup and when dma_active is written to 1.
	 */
	unsigned dma_error : 1;

	/**
	 * \brief Whether a DMA operation is in progress.
	 *
	 * Initialized to 0 at startup.
	 * A write to 1 starts a DMA transfer.
	 * A write to 0 aborts an in-progress DMA transfer.
	 * A read returns 1 when a DMA transfer is in progress, or 0 if no transfer has been started or if the last transfer has finished.
	 *
	 * In case of a write, this field may clear while spi_active is still set.
	 * Software may overwrite the DMA buffer at this time, but must not start bus activity until spi_active also clears.
	 */
	unsigned dma_active : 1;

	/**
	 * \brief Whether the next register access is short (0) or long (1).
	 */
	unsigned long_address : 1;

	/**
	 * \brief Whether to read (0) or write (1) the next register access.
	 */
	unsigned write : 1;

	/**
	 * \brief Whether or not a transaction is being executed on the SPI bus.
	 *
	 * Initialized to 0 at startup.
	 * A write to 1 starts a transaction.
	 */
	unsigned spi_active : 1;

	/**
	 * \brief Whether or not interrupt is asserted.
	 */
	unsigned interrupt : 1;

	/**
	 * \brief Whether or not wake is asserted.
	 *
	 * Initialized to 0 at startup.
	 */
	unsigned wake : 1;

	/**
	 * \brief Whether or not reset is asserted.
	 *
	 * Initialized to 0 at startup.
	 */
	unsigned reset : 1;
} io_mrf_csr_t;

/**
 * \brief The type of the registers controlling the MRF SPI port.
 */
typedef struct {
	/**
	 * \brief The control and status register.
	 *
	 * This register should not be written while DMA is active except to abort the transfer.
	 */
	io_mrf_csr_t csr;

	/**
	 * \brief The transmit/receive buffer.
	 *
	 * On a write, the lower 8 bits are stored for sending on next write to csr.spi_active.
	 * On a read, the last byte received in a transfer is returned.
	 */
	unsigned data;

	/**
	 * \brief Selects the register address that will be read or written on the next transaction.
	 *
	 * This register should not be written while DMA is active.
	 */
	unsigned address;

	/**
	 * \brief Selects the memory location for the next DMA transfer.
	 *
	 * This register should not be written while DMA is active.
	 */
	void *dma_address;

	/**
	 * \brief Selects the number of bytes left in DMA transfers.
	 *
	 * This register should not be written while DMA is active.
	 */
	unsigned dma_length;
} io_mrf_t;

/**
 * \brief The MRF SPI port.
 */
#define IO_MRF (*(volatile io_mrf_t *) (IO_APB_BASE + 0x00200))



/**
 * \brief The type of the control and status register for a motor.
 */
typedef struct {
	unsigned : 26;

	/**
	 * \brief Clears all sensor failure latches when written to 1.
	 *
	 * This bit is write-only and instantly clears.
	 */
	unsigned clear_failures : 1;

	/**
	 * \brief Reports whether the motor’s associated optical encoder, if any, is failing to commutate.
	 *
	 * This bit is read-only.
	 */
	unsigned encoder_failed : 1;

	/**
	 * \brief Reports whether the motor’s Hall sensors are currently stuck high.
	 *
	 * This bit is read-only.
	 */
	unsigned hall_stuck_high : 1;

	/**
	 * \brief Reports whether the motor’s Hall sensors are currently stuck low.
	 *
	 * This bit is read-only.
	 */
	unsigned hall_stuck_low : 1;

	/**
	 * \brief Selects the mode in which the motor is driven.
	 *
	 * Possible values are 0 (manual commutation), 1 (brake), 2 (forward), and 3 (reverse).
	 * These bits are read-write and initialized to 0 at startup.
	 */
	unsigned mode : 2;
} io_motor_csr_t;

/**
 * \brief The type of the registers controlling a motor.
 */
typedef struct {
	/**
	 * \brief The control and status register.
	 */
	io_motor_csr_t csr;

	/**
	 * \brief The manual commutation control register.
	 *
	 * The bottom six bits of this register are implemented, two bits per motor phase.
	 * Possible values for each two bits are 0 (float), 1 (PWM), 2 (fixed low), and 3 (fixed high).
	 * Initialized to 0 at startup.
	 * This register is always forced to 0 if interlocks are enabled.
	 */
	unsigned manual_commutation_pattern;

	/**
	 * \brief The PWM duty cycle applied to the motor.
	 *
	 * The bottom eight bits of this register are implemented.
	 * Initialized to 0 at startup.
	 */
	unsigned pwm;

	/**
	 * \brief The accumulated position of the motor, based on whatever positional feedback is available (optical encoder or Hall sensors).
	 *
	 * The bottom 16 bits of this register are implemented.
	 * This register is read-only.
	 */
	unsigned position;

	/**
	 * \brief The raw sensor values from all available sensors.
	 *
	 * The bottom 3 bits show the Hall sensors.
	 * The next 2 bits show the optical encoder lines, if implemented.
	 */
	unsigned sensors;
} io_motor_t;

/**
 * \brief The control registers for the motors, indexed by motor number.
 */
#define IO_MOTOR(N) (*(volatile io_motor_t *) (IO_APB_BASE + (0x00300 + ((N) << 8))))



/**
 * \brief The type of the control and status register for the chicker.
 */
typedef struct {
	unsigned : 25;

	/**
	 * \brief Whether or not the chicker board is present.
	 *
	 * This bit is read-only.
	 */
	unsigned present : 1;

	/**
	 * \brief Whether or not a chip pulse is currently executing.
	 *
	 * This bit is read-write and initialized to 0.
	 */
	unsigned chip : 1;

	/**
	 * \brief Whether or not a kick pulse is currently executing.
	 *
	 * This bit is read-write and initialized to 0.
	 */
	unsigned kick : 1;

	/**
	 * \brief Whether a charge timeout has occurred.
	 *
	 * This bit is read-only and initialized to 0.
	 */
	unsigned charge_timeout : 1;

	/**
	 * \brief Whether charging is complete.
	 *
	 * This bit is read-only and initialized to 0.
	 */
	unsigned charge_done : 1;

	/**
	 * \brief Whether safe discharge is enabled.
	 *
	 * This bit is read-write and initialized to 1.
	 */
	unsigned discharge : 1;

	/**
	 * \brief Whether charging is enabled.
	 *
	 * This bit is read-write and initialized to 0.
	 */
	unsigned charge : 1;
} io_chicker_csr_t;

/**
 * \brief The type of the registers controlling the chicker.
 */
typedef struct {
	/**
	 * \brief The control and status register.
	 */
	io_chicker_csr_t csr;

	/**
	 * \brief The capacitor voltage, in ADC units.
	 */
	const unsigned capacitor_voltage;

	/**
	 * \brief The width of pulse to send to the kicker or chipper when firing, in microseconds.
	 */
	unsigned pulse_width;
} io_chicker_t;

/**
 * \brief The control registers for the chicker.
 */
#define IO_CHICKER (*(volatile io_chicker_t *) (IO_APB_BASE + 0x00800))



/**
 * \brief The type of the control and status register in the SD host controller.
 */
typedef struct {
	unsigned : 24;

	/**
	 * \brief Whether a DMA operation is occurring.
	 *
	 * This bit is read-write and initialized to 0.
	 */
	unsigned dma_enable : 1;

	/**
	 * \brief Whether an AHB error occurred during the last DMA transfer, which will thus not have completed.
	 *
	 * This bit is also set if a DMA transfer is enabled with an improper length.
	 *
	 * This bit is read-only and initialized to 0 every time dma_enable is written to 1.
	 */
	unsigned ahb_error : 1;

	/**
	 * \brief Whether a malformed data response token was seen during DMA.
	 *
	 * This bit is read-only and initialized to 0 every time dma_enable is written to 1.
	 */
	unsigned malformed_drt : 1;

	/**
	 * \brief Whether the SD card reported an internal error in a data response token during DMA.
	 *
	 * This bit is read-only and initialized to 0 every time dma_enable is written to 1.
	 */
	unsigned internal_error : 1;

	/**
	 * \brief Whether the SD card reported a CRC error in a data response token during DMA.
	 *
	 * This bit is read-only and initialized to 0 every time dma_enable is written to 1.
	 */
	unsigned crc_error : 1;

	/**
	 * \brief Controls the chip select wire to the SD card.
	 *
	 * This bit is read-write and initialized to 1.
	 */
	unsigned cs : 1;

	/**
	 * \brief Whether or not the SD card is present.
	 *
	 * This bit is read-only.
	 */
	unsigned present : 1;

	/**
	 * \brief Whether the SPI bus is busy.
	 *
	 * This bit is read-only and initialized to 0.
	 */
	unsigned busy : 1;
} io_sd_csr_t;

/**
 * \brief The type of the control registers for the Secure Digital host controller.
 */
typedef struct {
	/**
	 * \brief The control and status register.
	 */
	io_sd_csr_t csr;

	/**
	 * \brief The data register.
	 *
	 * A read returns the data from the most recent bus transaction.
	 * A write initiates a bus transaction sending the low 8 bits of the written data.
	 */
	unsigned data;

	/**
	 * \brief The address of the next byte to send to the card when DMA is being used.
	 */
	const void *dma_address;

	/**
	 * \brief The number of bytes remaining to send to the card in a DMA operation.
	 *
	 * This must currently always be set to 512 when starting a DMA operation.
	 */
	unsigned dma_length;
} io_sd_t;

/**
 * \brief The control registers for the Secure Digital host controller.
 */
#define IO_SD (*(volatile io_sd_t *) (IO_APB_BASE + 0x00900))



/**
 * \brief The type of an identification register in the AHB or APB configuration space.
 */
typedef struct {
	/**
	 * \brief The vendor ID.
	 */
	unsigned vendor_id : 8;

	/**
	 * \brief The device ID.
	 */
	unsigned device_id : 12;

	/**
	 * \brief The version number of the configuration data record.
	 */
	unsigned config_version : 2;

	/**
	 * \brief The version number of the device.
	 */
	unsigned version : 5;

	/**
	 * \brief The number of the first interrupt line the device uses, if any.
	 */
	unsigned irq : 5;
} io_pnp_devid_t;

/**
 * \brief The type of a base address register in the AHB or APB configuration space.
 */
typedef struct {
	/**
	 * \brief The address bits (either the top 12 bits, for an AHB memory BAR, or the next 12 bits, for an AHB or APB I/O BAR).
	 */
	unsigned address : 12;

	unsigned : 2;

	/**
	 * \brief Whether or not these addresses are prefetchable.
	 */
	unsigned int prefetchable : 1;

	/**
	 * \brief Whether or not these addresses are cacheable.
	 */
	unsigned int cacheable : 1;

	/**
	 * \brief Which bits of address must match to address the device.
	 */
	unsigned mask : 12;

	/**
	 * \brief The type of address range (1 for APB I/O, 2 for AHB memory, or 3 for AHB I/O).
	 */
	unsigned type : 4;
} io_pnp_bar_t;

/**
 * \brief The type of a complete AHB plug-and-play data record.
 */
typedef struct {
	io_pnp_devid_t devid;
	unsigned int user1;
	unsigned int user2;
	unsigned int user3;
	io_pnp_bar_t bars[4];
} io_pnp_ahb_device_t;

/**
 * \brief The type of the full set of AHB plug-and-play records.
 */
typedef struct {
	io_pnp_ahb_device_t masters[64];
	io_pnp_ahb_device_t slaves[64];
} io_pnp_ahb_t;

/**
 * \brief The type of a complete APB plug-and-play data record.
 */
typedef struct {
	io_pnp_devid_t devid;
	io_pnp_bar_t bar;
} io_pnp_apb_device_t;

/**
 * \brief The type of a full set of APB plug-and-play records.
 */
typedef io_pnp_apb_device_t io_pnp_apb_t[512];

/**
 * \brief The AHB plug-and-play configuration data.
 */
#define IO_AHB_PNP_DATA (*(const io_pnp_ahb_t *) (IO_AHB_BASE + 0xFF000))

/**
 * \brief The APB plug-and-play configuration data.
 */
#define IO_APB_PNP_DATA (*(const io_pnp_apb_t *) (IO_APB_BASE + 0xFF000))

#endif

