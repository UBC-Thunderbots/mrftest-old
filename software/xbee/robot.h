#ifndef XBEE_ROBOT_H
#define XBEE_ROBOT_H

#include "util/async_operation.h"
#include "util/bit_array.h"
#include "util/byref.h"
#include "util/property.h"
#include <cstddef>
#include <stdint.h>

class XBeeDongle;

/**
 * \brief A single robot addressable through a dongle.
 */
class XBeeRobot : public ByRef {
	public:
		/**
		 * \brief A pointer to an XBeeRobot.
		 */
		typedef RefPtr<XBeeRobot> Ptr;

		/**
		 * \brief The components of the operational parameters block stored on board a robot.
		 */
		struct OperationalParameters {
			enum class FlashContents {
				FPGA,
				PIC,
				NONE,
			} flash_contents;

			uint8_t xbee_channels[2];

			uint8_t robot_number;

			uint8_t dribble_power;
		};

		/**
		 * \brief The components of a build signature block identifying firmware and FPGA bitstream versions.
		 */
		struct BuildSignatures {
			uint16_t firmware_signature;
			uint16_t flash_signature;
		};

		/**
		 * \brief The index of the robot, from 0 to 15.
		 */
		const unsigned int index;

		/**
		 * \brief Whether or not the robot is currently responding to radio communication.
		 */
		Property<bool> alive;

		/**
		 * \brief Whether or not up-to-date feedback is available for the robot.
		 */
		Property<bool> has_feedback;

		/**
		 * \brief Whether or not the ball is interrupting the robot's laser beam.
		 */
		Property<bool> ball_in_beam;

		/**
		 * \brief Whether or not the ball is loading the robot's dribbler motor.
		 */
		Property<bool> ball_on_dribbler;

		/**
		 * \brief Whether or not the robot's capacitor is charged enough to kick the ball.
		 */
		Property<bool> capacitor_charged;

		/**
		 * \brief The voltage on the robot's battery, in volts.
		 */
		Property<double> battery_voltage;

		/**
		 * \brief The voltage on the robot's kicking capacitor, in volts.
		 */
		Property<double> capacitor_voltage;

		/**
		 * \brief The temperature of the robot's dribbler motor, in degrees Celsius.
		 */
		Property<double> dribbler_temperature;

		/**
		 * \brief The raw analogue-to-digital converter reading of the robot's laser sensor.
		 */
		Property<unsigned int> break_beam_reading;

		/**
		 * \brief Erases the SPI flash chip on the robot.
		 *
		 * \return an asynchronous operation whose progress can be monitored.
		 */
		AsyncOperation<void>::Ptr firmware_spi_chip_erase();

		/**
		 * \brief Copies a block of data into the holding buffer on board the robot in preparation for writing it to the SPI flash chip.
		 *
		 * \param[in] offset the offset within the holding buffer at which the data should be written.
		 *
		 * \param[in] data the data to write.
		 *
		 * \param[in] length the number of bytes to write.
		 *
		 * \return an asynchronous operation whose progress can be monitored.
		 */
		AsyncOperation<void>::Ptr firmware_spi_fill_page_buffer(unsigned int offset, const void *data, std::size_t length);

		/**
		 * \brief Writes the contents of the robot's on-board holding buffer to the SPI flash chip.
		 *
		 * \param[in] page the page number on the SPI flash chip at which the buffer should be written.
		 *
		 * \param[in] crc a CRC-16 of the entire contents of the holding buffer, which must match for the operation to succeed.
		 *
		 * \return an asynchronous operation whose progress can be monitored.
		 */
		AsyncOperation<void>::Ptr firmware_spi_page_program(unsigned int page, uint16_t crc);

		/**
		 * \brief Computes the CRC-16 of a block of data in the robot's SPI flash chip.
		 *
		 * \param[in] address the address of the first byte to checksum.
		 *
		 * \param[in] length the number of bytes to checksum.
		 *
		 * \return an asynchronous operation whose progress can be monitored and from which the CRC-16 can be fetched.
		 */
		AsyncOperation<uint16_t>::Ptr firmware_spi_block_crc(unsigned int address, std::size_t length);

		/**
		 * \brief Reads the on-board operational parameters from the robot.
		 *
		 * \return an asynchronous operation whose progress can be monitored and from which the operational parameters block can be fetched.
		 */
		AsyncOperation<OperationalParameters>::Ptr firmware_read_operational_parameters();

		/**
		 * \brief Writes the on-board operational parameters on the robot.
		 *
		 * The new parameters are stored in RAM but are not committed to non-volatile storage.
		 *
		 * \param[in] params the new parameters to apply.
		 *
		 * \return an asynchronous operation whose progress can be monitored.
		 */
		AsyncOperation<void>::Ptr firmware_set_operational_parameters(const OperationalParameters &params);

		/**
		 * \brief Copies the on-board operational parameters on the robot from RAM to non-volatile storage.
		 *
		 * \return an asynchronous operation whose progress can be monitored.
		 */
		AsyncOperation<void>::Ptr firmware_commit_operational_parameters();

		/**
		 * \brief Reboots the robot.
		 *
		 * \return an asynchronous operation whose progress can be monitored.
		 */
		AsyncOperation<void>::Ptr firmware_reboot();

		/**
		 * \brief Reads the build signatures of the PIC firmware and FPGA bitstream from the robot.
		 *
		 * \return an asynchronous operation whose progress can be monitored and from which the build signatures can be fetched.
		 */
		AsyncOperation<BuildSignatures>::Ptr firmware_read_build_signatures();

		/**
		 * \brief Sets the speeds of the robot's wheels.
		 *
		 * \param[in] wheels the speeds of the wheels, in quarters of a degree per five milliseconds, in the range ±1023.
		 */
		void drive(const int(&wheels)[4]);

		/**
		 * \brief Halts the robot's wheels.
		 */
		void drive_scram();

		/**
		 * \brief Turns the dribbler motor on or off.
		 *
		 * \param[in] active \c true to turn the motor on, or \c false to turn it off.
		 */
		void dribble(bool active = true);

		/**
		 * \brief Turns the capacitor charger on or off.
		 *
		 * \param[in] active \c true to turn the charger on, or \c false to turn it off.
		 */
		void enable_chicker(bool active = true);

		/**
		 * \brief Executes a kick.
		 *
		 * \param[in] pulse_width1 the width of the pulse to send to solenoid #1, in microseconds.
		 *
		 * \param[in] pulse_width2 the width of the pulse to send to solenoid #2, in microseconds.
		 *
		 * \param[in] offset the time difference between the leading edges of the pulses, in microseconds, with positive values causing solenoid #1's pulse to lag solenoid #2's pulse.
		 */
		void kick(unsigned int pulse_width1, unsigned int pulse_width2, int offset);

		/**
		 * \brief Enables or disables automatic kicking when the ball breaks the robot's laser.
		 *
		 * If all parameters are set to zero, automatic kicking will be disabled.
		 *
		 * \param[in] pulse_width1 the width of the pulse to send to solenoid #1, in microseconds.
		 *
		 * \param[in] pulse_width2 the width of the pulse to send to solenoid #2, in microseconds.
		 *
		 * \param[in] offset the time difference between the leading edges of the pulses, in microseconds, with positive values causing solenoid #1's pulse to lag solenoid #2's pulse.
		 */
		void autokick(unsigned int pulse_width1, unsigned int pulse_width2, int offset);

		/**
		 * \brief Sets an on-board electrical test readout mode on the robot's LEDs.
		 *
		 * \param[in] mode the mode number to apply.
		 */
		void test_mode(unsigned int mode);

	private:
		friend class XBeeDongle;

		XBeeDongle &dongle;
		BitArray<80> drive_block;

		static Ptr create(XBeeDongle &dongle, unsigned int index);
		XBeeRobot(XBeeDongle &dongle, unsigned int index);
		void flush_drive();
		void on_feedback(const uint8_t *data, std::size_t length);
};

#endif

