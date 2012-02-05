#ifndef XBEE_ROBOT_H
#define XBEE_ROBOT_H

#include "util/annunciator.h"
#include "util/async_operation.h"
#include "util/bit_array.h"
#include "util/noncopyable.h"
#include "util/property.h"
#include "xbee/dongle.h"
#include "xbee/drivepacket.h"
#include <cstddef>
#include <functional>
#include <memory>
#include <stdint.h>

/**
 * \brief A single robot addressable through a dongle.
 */
class XBeeRobot : public NonCopyable {
	public:
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
		 * \brief The possible states of the charger.
		 */
		enum class ChargerState {
			/**
			 * \brief Safely discharges the capacitor down to battery level.
			 */
			DISCHARGE,

			/**
			 * \brief Neither charges nor discharges the capacitors.
			 */
			FLOAT,

			/**
			 * \brief Charges the capacitor to full voltage.
			 */
			CHARGE,
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
		 * \brief Emitted when the robot sends a block of experiment data.
		 */
		sigc::signal<void, const void *, std::size_t> signal_experiment_data;

		/**
		 * \brief Emitted when the autokick mechanism causes the robot to kick.
		 */
		sigc::signal<void> signal_autokick_fired;

		/**
		 * \brief An operation to erase the SPI flash chip on the robot.
		 */
		class FirmwareSPIChipEraseOperation;

		/**
		 * \brief An operation to copy a block of data into the holding buffer on board the robot.
		 *
		 * Data must be copied to the holding buffer in blocks before it can be written to the flash chip.
		 */
		class FirmwareSPIFillPageBufferOperation;

		/**
		 * \brief An operation to commit the contents of the on-board holding buffer to the SPI flash chip.
		 */
		class FirmwareSPIPageProgramOperation;

		/**
		 * \brief An operation to compute the CRC-16 of a block of data in the robot's SPI flash chip.
		 */
		class FirmwareSPIBlockCRCOperation;

		/**
		 * \brief An operation to read the on-board operational parameters from the robot.
		 */
		class FirmwareReadOperationalParametersOperation;

		/**
		 * \brief An operation to write the on-board operational parameters on the robot.
		 *
		 * The new parameters are stored in RAM but are not committed to non-volatile storage.
		 */
		class FirmwareWriteOperationalParametersOperation;

		/**
		 * \brief An operation to copy the on-board operational parameters on the robot from RAM to non-volatile storage.
		 */
		class FirmwareCommitOperationalParametersOperation;

		/**
		 * \brief An operation to reboot the robot.
		 */
		class RebootOperation;

		/**
		 * \brief An operation to read the build signatures of the PIC firmware and FPGA bitstream from the robot.
		 */
		class FirmwareReadBuildSignaturesOperation;

		/**
		 * \brief Sets the speeds of the robot's wheels.
		 *
		 * \param[in] wheels the speeds of the wheels, in quarters of a degree per five milliseconds, in the range ±1023.
		 *
		 * \param[in] controlled \c true to run the provided setpoints through the wheel controllers, or \c false to run open-loop.
		 */
		void drive(const int(&wheels)[4], bool controlled = true);

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
		 * \brief Sets the state of the capacitor charger.
		 *
		 * \param[in] state the state to set the charger to.
		 */
		void set_charger_state(ChargerState state);

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

		/**
		 * \brief Starts a scripted experiment.
		 *
		 * \param[in] control_code the control code for configuring the experiment.
		 */
		void start_experiment(uint8_t control_code);

	private:
		friend class XBeeDongle;

		XBeeDongle &dongle;
		XBeePackets::Drive drive_block, last_drive_block;
		Annunciator::Message encoder_1_stuck_message, encoder_2_stuck_message, encoder_3_stuck_message, encoder_4_stuck_message;
		Annunciator::Message hall_stuck_message;
		std::unique_ptr<XBeeDongle::SendMessageOperation> kick_send_message_op, test_mode_send_message_op, start_experiment_send_message_op;

		XBeeRobot(XBeeDongle &dongle, unsigned int index);
		void flush_drive(bool force = false);
		void on_feedback(const uint8_t *data, std::size_t length);
		void check_kick_message_result(AsyncOperation<void> &);
		void check_test_mode_message_result(AsyncOperation<void> &);
		void check_start_experiment_message_result(AsyncOperation<void> &);
};



class XBeeRobot::FirmwareSPIChipEraseOperation : public AsyncOperation<void>, public sigc::trackable {
	public:
		/**
		 * \brief Starts the chip erase operation.
		 *
		 * \param[in] robot the robot to send the command to.
		 */
		FirmwareSPIChipEraseOperation(XBeeRobot &robot);

		/**
		 * \brief Checks for the success of the operation.
		 *
		 * If the operation failed, this function throws the relevant exception.
		 */
		void result() const;

	private:
		XBeeRobot &robot;
		uint8_t buffer[2];
		XBeeDongle::SendMessageOperation send_message_op;
		sigc::connection receive_message_conn;

		void send_message_op_done(AsyncOperation<void> &op);
		void check_received_message(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len);
};

class XBeeRobot::FirmwareSPIFillPageBufferOperation : public AsyncOperation<void>, public sigc::trackable {
	public:
		/**
		 * \brief Starts the buffer-fill operation.
		 *
		 * \param[in] robot the robot to send the command to.
		 *
		 * \param[in] offset the offset within the holding buffer at which the data should be written.
		 *
		 * \param[in] data the data to write (the data is copied into an internal buffer).
		 *
		 * \param[in] length the number of bytes to write.
		 *
		 * \pre \p length ≤ 61
		 */
		FirmwareSPIFillPageBufferOperation(XBeeRobot &robot, unsigned int offset, const void *data, std::size_t length);

		/**
		 * \brief Checks for the success of the operation.
		 *
		 * If the operation failed, this function throws the relevant exception.
		 */
		void result() const;

	private:
		XBeeRobot &robot;
		uint8_t buffer[64];
		XBeeDongle::SendMessageOperation send_message_op;

		uint8_t *prepare_buffer(unsigned int offset, const void *data, std::size_t length);
		void send_message_op_done(AsyncOperation<void> &);
};

class XBeeRobot::FirmwareSPIPageProgramOperation : public AsyncOperation<void>, public sigc::trackable {
	public:
		/**
		 * \brief Starts the page program operation.
		 *
		 * \param[in] robot the robot to send the command to.
		 *
		 * \param[in] page the page number on the SPI flash chip at which the buffer should be written.
		 *
		 * \param[in] crc a CRC-16 of the entire contents of the holding buffer, which must match for the operation to succeed.
		 */
		FirmwareSPIPageProgramOperation(XBeeRobot &robot, unsigned int page, uint16_t crc);

		/**
		 * \brief Checks for the success of the operation.
		 *
		 * If the operation failed, this function throws the relevant exception.
		 */
		void result() const;

	private:
		XBeeRobot &robot;
		uint8_t buffer[6];
		XBeeDongle::SendMessageOperation send_message_op;
		sigc::connection receive_message_conn;

		void send_message_op_done(AsyncOperation<void> &op);
		void check_received_message(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len);
};

class XBeeRobot::FirmwareSPIBlockCRCOperation : public AsyncOperation<uint16_t>, public sigc::trackable {
	public:
		/**
		 * \brief Starts the CRC operation.
		 *
		 * \param[in] robot the robot to send the command to.
		 *
		 * \param[in] address the address of the first byte to checksum.
		 *
		 * \param[in] length the number of bytes to checksum.
		 */
		FirmwareSPIBlockCRCOperation(XBeeRobot &robot, unsigned int address, std::size_t length);

		/**
		 * \brief Checks for the success of the operation and returns the computed CRC.
		 *
		 * If the operation failed, this function throws the relevant exception.
		 *
		 * \return the CRC of the block.
		 */
		uint16_t result() const;

	private:
		XBeeRobot &robot;
		unsigned int address;
		std::size_t length;
		uint8_t buffer[7];
		XBeeDongle::SendMessageOperation send_message_op;
		sigc::connection receive_message_conn;
		uint16_t crc;

		void send_message_op_done(AsyncOperation<void> &op);
		void check_received_message(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len);
};

class XBeeRobot::FirmwareReadOperationalParametersOperation : public AsyncOperation<OperationalParameters>, public sigc::trackable {
	public:
		/**
		 * \brief Starts the parameter read operation.
		 *
		 * \param[in] robot the robot to send the command to.
		 *
		 * \return an asynchronous operation whose progress can be monitored and from which the operational parameters block can be fetched.
		 */
		FirmwareReadOperationalParametersOperation(XBeeRobot &robot);

		/**
		 * \brief Checks for the success of the operation and returns the operational parameters.
		 *
		 * If the operation failed, this function throws the relevant exception.
		 *
		 * \return the operational parameters.
		 */
		OperationalParameters result() const;

	private:
		XBeeRobot &robot;
		uint8_t buffer[2];
		XBeeDongle::SendMessageOperation send_message_op;
		sigc::connection receive_message_conn;
		OperationalParameters params;

		void send_message_op_done(AsyncOperation<void> &op);
		void check_received_message(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len);
};

class XBeeRobot::FirmwareWriteOperationalParametersOperation : public AsyncOperation<void>, public sigc::trackable {
	public:
		/**
		 * \brief Starts the operation.
		 *
		 * \param[in] robot the robot to send the command to.
		 *
		 * \param[in] params the new parameters to apply (the data is copied into an internal buffer).
		 *
		 * \return an asynchronous operation whose progress can be monitored.
		 */
		FirmwareWriteOperationalParametersOperation(XBeeRobot &robot, const OperationalParameters &params);

		/**
		 * \brief Checks for the success of the operation.
		 *
		 * If the operation failed, this function throws the relevant exception.
		 */
		void result() const;

	private:
		XBeeRobot &robot;
		uint8_t buffer[8];
		XBeeDongle::SendMessageOperation send_message_op;
		sigc::connection receive_message_conn;

		uint8_t *prepare_buffer(unsigned int offset, const void *data, std::size_t length);
		void send_message_op_done(AsyncOperation<void> &);
		void check_received_message(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len);
};

class XBeeRobot::FirmwareCommitOperationalParametersOperation : public AsyncOperation<void>, public sigc::trackable {
	public:
		/**
		 * \brief Starts the operation.
		 *
		 * \param[in] robot the robot to send the command to.
		 *
		 * \return an asynchronous operation whose progress can be monitored.
		 */
		FirmwareCommitOperationalParametersOperation(XBeeRobot &robot);

		/**
		 * \brief Checks for the success of the operation.
		 *
		 * If the operation failed, this function throws the relevant exception.
		 */
		void result() const;

	private:
		XBeeRobot &robot;
		uint8_t buffer[2];
		XBeeDongle::SendMessageOperation send_message_op;
		sigc::connection receive_message_conn;

		uint8_t *prepare_buffer(unsigned int offset, const void *data, std::size_t length);
		void send_message_op_done(AsyncOperation<void> &);
		void check_received_message(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len);
};

class XBeeRobot::RebootOperation : public AsyncOperation<void>, public sigc::trackable {
	public:
		/**
		 * \brief Starts the operation.
		 *
		 * \param[in] robot the robot to send the command to.
		 */
		RebootOperation(XBeeRobot &robot);

		/**
		 * \brief Checks for the success of the operation.
		 *
		 * If the operation failed, this function throws the relevant exception.
		 */
		void result() const;

	private:
		XBeeRobot &robot;
		uint8_t buffer[2];
		XBeeDongle::SendMessageOperation send_message_op;
		sigc::connection receive_message_conn;

		uint8_t *prepare_buffer(unsigned int offset, const void *data, std::size_t length);
		void send_message_op_done(AsyncOperation<void> &);
		void check_received_message(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len);
};

class XBeeRobot::FirmwareReadBuildSignaturesOperation : public AsyncOperation<BuildSignatures>, public sigc::trackable {
	public:
		/**
		 * \brief Starts the operation.
		 *
		 * \param[in] robot the robot to send the command to.
		 */
		FirmwareReadBuildSignaturesOperation(XBeeRobot &robot);

		/**
		 * \brief Checks for the success of the operation and returns the build signatures.
		 *
		 * If the operation failed, this function throws the relevant exception.
		 *
		 * \return the build signatures.
		 */
		BuildSignatures result() const;

	private:
		XBeeRobot &robot;
		uint8_t buffer[2];
		XBeeDongle::SendMessageOperation send_message_op;
		sigc::connection receive_message_conn;
		BuildSignatures sigs;

		void send_message_op_done(AsyncOperation<void> &op);
		void check_received_message(unsigned int robot, XBeeDongle::Pipe pipe, const void *data, std::size_t len);
};

#endif

