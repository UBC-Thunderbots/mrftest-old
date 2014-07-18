#ifndef ICB_H
#define ICB_H

#include <FreeRTOS.h>
#include <semphr.h>
#include <stddef.h>

/**
 * \ingroup ICB
 *
 * \brief The available ICB commands.
 */
typedef enum {
	/**
	 * \brief Read device DNA.
	 *
	 * \return 7 byte device DNA, LSB first
	 */
	ICB_COMMAND_READ_DNA = 0x00,

	/**
	 * \brief Read configuration switches.
	 *
	 * Interpretation fo the flags byte is as follows:
	 *
	 * <table>
	 * <tr><td>Bit</td><td>Meaning</td></tr>
	 * <tr><td>0</td><td>Radio channel index (0=primary, 1=alternate)</td></tr>
	 * <tr><td>1</td><td>Safety interlock status (0=overridden, 1=active)</td></tr>
	 * <tr><td>2–7</td><td>Unused</td></tr>
	 * </table>
	 *
	 * \return 1 byte robot index plus 1 byte flags
	 */
	ICB_COMMAND_READ_SWITCHES = 0x01,

	/**
	 * \brief Write LEDs.
	 *
	 * Interpretation of the parameters is as follows:
	 *
	 * <table>
	 * <tr><td>\p mode</td><td>\p value</td></tr>
	 * <tr><td>0</td><td>bits to show on LEDs</td></tr>
	 * <tr><td>1</td><td>index of motor whose Hall sensors to show on LEDs</td></tr>
	 * </table>
	 *
	 * \param mode 1 byte
	 * \param value 1 byte
	 */
	ICB_COMMAND_WRITE_LEDS = 0x02,

	/**
	 * \brief Read and simultaneously clear interrupt flags.
	 *
	 * \return ceil(nIRQs/8) bytes IRQ flags, LSB first
	 */
	ICB_COMMAND_GET_CLEAR_IRQS = 0x10,

	/**
	 * \brief Start reading short-address MRF register.
	 *
	 * An interrupt occurs when the read finishes.
	 *
	 * \param address 1 byte register to read
	 */
	ICB_COMMAND_MRF_DA_READ_SHORT = 0x20,

	/**
	 * \brief Start writing short-address MRF register.
	 *
	 * An interrupt occurs when the write finishes.
	 *
	 * \param address 1 byte register to write
	 * \param data 1 byte data to write
	 */
	ICB_COMMAND_MRF_DA_WRITE_SHORT = 0x21,

	/**
	 * \brief Start reading long-address MRF register.
	 *
	 * An interrupt occurs when the read finishes.
	 *
	 * \param address 2 bytes register to read
	 */
	ICB_COMMAND_MRF_DA_READ_LONG = 0x22,

	/**
	 * \brief Start writing long-address MRF register.
	 *
	 * An interrupt occurs when the write finishes.
	 *
	 * \param address 2 bytes register to write
	 * \param data 1 byte data to write
	 */
	ICB_COMMAND_MRF_DA_WRITE_LONG = 0x23,

	/**
	 * \brief Read out data from last direct-access command.
	 *
	 * This command should be issued when an \ref ICB_COMMAND_MRF_DA_READ_SHORT or \ref ICB_COMMAND_MRF_DA_READ_LONG finishes.
	 *
	 * \return 1 byte data
	 */
	ICB_COMMAND_MRF_DA_GET_DATA = 0x24,

	/**
	 * \brief Reads the hardware interrupt pin.
	 *
	 * \return 1 byte interrupt line status, 1 = high, 0 = low
	 */
	ICB_COMMAND_MRF_DA_GET_INT = 0x25,

	/**
	 * \brief Writes the hardware auxiliary pins.
	 *
	 * \param data 1 byte auxiliary pin states (bit 0 = /RESET, bit 1 = WAKE)
	 */
	ICB_COMMAND_MRF_DA_SET_AUX = 0x26,

	/**
	 * \brief Enables the MRF offload engines.
	 *
	 * There are three offload engines:
	 * \li The interrupt offload engine is triggered by the hardware interrupt line being high and reads and presents the value of \c INTSTAT for the other engines.
	 * \li The receive offload engine is triggered whenever a frame is received and reads the frame out of the receive FIFO into the FPGA.
	 * \li The transmit offload engine is triggered by an ICB command and copies the frame into the transmit FIFO, triggers it, and retrieves transmit status when finished.
	 *
	 * At powerup, all offload engines are disabled.
	 * Once the engines are enabled, they cannot be disabled.
	 * Also, once the engines are enabled, the firmware should not use the direct access commands to read \c INTSTAT, as doing so may lock up the engines.
	 *
	 * \pre The radio must be configured to use its hardware interrupt line in active-low mode.
	 * \pre It must not be the case that a receive interrupt has been acknowledged (by reading \c INTSTAT) but the received frame has not been read out from the radio’s receive FIFO.
	 * \pre There must not be a frame currently transmitting.
	 * \pre There must not be an unacknowledged transmit complete interrupt.
	 * \pre The radio must be configured to include a link quality indication in the receive FIFO
	 */
	ICB_COMMAND_MRF_OFFLOAD = 0x27,

	/**
	 * \brief Gets the size of the next frame in the MRF receive offload engine queue.
	 *
	 * \return 1 byte the length of the next frame, or zero if no frames are waiting
	 */
	ICB_COMMAND_MRF_RX_GET_SIZE = 0x28,

	/**
	 * \brief Reads the next frame from the MRF receive offload engine queue.
	 *
	 * \return \p n bytes the frame data, which includes the 802.15.4 header, application payload, 2-byte FCS, 1-byte LQI, and 1-byte RSSI
	 */
	ICB_COMMAND_MRF_RX_READ = 0x29,

	/**
	 * \brief Pushes a frame to the MRF transmit offload engine queue.
	 *
	 * \param hlength 1 byte length of IEEE 802.15.4 header
	 * \param length 1 byte length of frame data (header plus payload)
	 * \param header \p hlength bytes IEEE 802.15.4 header
	 * \param payload \p length − \p hlength bytes application payload
	 *
	 * \pre The transmit offload engine queue must have enough space available to hold the packet (the application is responsible for tracking space in the queue based on pushes and completions).
	 */
	ICB_COMMAND_MRF_TX_PUSH = 0x2A,

	/**
	 * \brief Retrieves a queued status code.
	 *
	 * \return 1 byte the \c TXSTAT register contents when transmission finished
	 *
	 * \pre A transmit complete interrupt must have occurred.
	 */
	ICB_COMMAND_MRF_TX_GET_STATUS = 0x2B,

	/**
	 * \brief Disables the MRF offload engines.
	 *
	 * The offload engines’ internal buffers are flushed, discarding any packet contained therein.
	 *
	 * The state of the radio itself is undefined with respect to those aspects that the offload engines control:
	 * \li A received packet may or may not be left in the receive buffer.
	 * \li A packet being transmitted may or may not be left in the transmit buffer.
	 * \li The radio may or may not be currently transmitting.
	 * \li Interrupt causes may or may not have been read from \c INTSTAT but not yet handled.
	 *
	 * After executing this command, the SPI bus to the radio may take a short time to complete its last transaction.
	 *
	 * It is recommended that the radio be reset (using its hardware reset wire) before continued operation.
	 */
	ICB_COMMAND_MRF_OFFLOAD_DISABLE = 0x2C,

	/**
	 * \brief Sets the operational parameters of the motors.
	 *
	 * A motor parameter block consists of two bytes.
	 * The first byte specifies the mode.
	 * The second byte specifies the PWM duty cycle.
	 *
	 * The mode byte is divided into bits as follows:
	 *
	 * <table>
	 * <tr><td>Bit</td><td>Purpose</td></tr>
	 * <tr><td>7</td><td>Commutation data source (0 = MCU, 1 = Hall sensors)</td></tr>
	 * <tr><td>6</td><td>Hall sensor commutation direction (0 = forward, 1 = reverse) (ignored if bit 7 is 0)</td></tr>
	 * <tr><td>5–4</td><td>Phase 2 drive mode (ignored if bit 7 is 1): 00 = float, 01 = PWM, 10 = drive low, 11 = drive high</td></tr>
	 * <tr><td>3–2</td><td>Phase 1 drive mode</td></tr>
	 * <tr><td>1–0</td><td>Phase 0 drive mode</td></tr>
	 * </table>
	 *
	 * Note that when safety interlocks are applied, only floating and driving low are possible in MCU-commutated mode; PWM and driving high are prohibited.
	 *
	 * \param motors 10 bytes, 5 motor parameter blocks
	 */
	ICB_COMMAND_MOTORS_SET = 0x30,

	/**
	 * \brief Reads the Hall-sensor-computed accumulated positions of the motors.
	 *
	 * \return 10 bytes, 5 16-bit integer accumulated counts of Hall sensor edges
	 */
	ICB_COMMAND_MOTORS_GET_HALL_COUNT = 0x31,

	/**
	 * \brief Reads a bitmask of which Hall sensors are stuck.
	 *
	 * The stuck state of Hall sensors is latched and is both reported and cleared by this transaction.
	 *
	 * \return 2 bytes, the first of which is which motors have sensors stuck low, the second of which is the same for stuck high
	 */
	ICB_COMMAND_MOTORS_GET_CLEAR_STUCK_HALLS = 0x32,

	/**
	 * \brief Read the Accelerometer data
	 *
	 * \return 6 bytes, 3 16-bit integer values one for each axis
	 */
	ICB_COMMAND_SENSORS_GET_ACCEL = 0x40,

	/**
	 * \brief Read the Gyro Data
	 *
	 * \return 6 bytes, 3 16-bit integer values one for each axis
	 */
	ICB_COMMAND_SENSORS_GET_GYRO = 0x41,
} icb_command_t;

/**
 * \ingroup ICB
 *
 * \brief The interrupt bits in the ICB interrupt controller.
 */
typedef enum {
	ICB_IRQ_MRF_DA, ///< The MRF direct access engine has finished accessing a register.
	ICB_IRQ_MRF_RX, ///< The MRF receive offload engine has received a packet.
	ICB_IRQ_MRF_TX, ///< The MRF transmit offload engine has finished trying to transmit a packet.
	ICB_IRQ_ICB_CRC, ///< An ICB transaction from MCU to FPGA had a bad CRC.
	ICB_IRQ_MRF_RX_FCS_FAIL, ///< The MRF receive offload engine has received a packet with bad FCS.
	ICB_IRQ_COUNT, ///< The number of interrupt bits (must be the last enumeration element).
} icb_irq_t;

/**
 * \ingroup ICB
 *
 * \brief The possible results of an FPGA configuration attempt or part thereof.
 */
typedef enum {
	ICB_CONF_OK, ///< No errors have occurred so far.
	ICB_CONF_INIT_B_STUCK_HIGH, ///< The INIT_B pin did not fall when PROGRAM_B went low.
	ICB_CONF_INIT_B_STUCK_LOW, ///< The INIT_B pin did not rise after PROGRAM_B went high.
	ICB_CONF_DONE_STUCK_HIGH, ///< The DONE pin did not read as low during configuration.
	ICB_CONF_DONE_STUCK_LOW, ///< The DONE pin did not go high after configuration.
	ICB_CONF_CRC_ERROR, ///< The INIT_B pin fell indicating a bitstream CRC error.
} icb_conf_result_t;

void icb_init(void);
void icb_send(icb_command_t command, const void *data, size_t length);
void icb_receive(icb_command_t command, void *buffer, size_t length);
bool icb_crc_error_check(void);
void icb_crc_error_clear(void);
void icb_irq_init(void);
void icb_irq_shutdown(void);
void icb_irq_set_vector(icb_irq_t irq, void (*isr)(void));
icb_conf_result_t icb_conf_start(void);
void icb_conf_block(const void *data, size_t length);
icb_conf_result_t icb_conf_end(void);
void dma2_stream0_isr(void);
void dma2_stream3_isr(void);
void exti0_isr(void);

#endif
