#ifndef XBEE_ROBOT_H
#define XBEE_ROBOT_H

#include "util/async_operation.h"
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
		typedef RefPtr<XBeeRobot> Ptr;

		struct OperationalParameters {
			enum FlashContents {
				FLASH_CONTENTS_NONE,
				FLASH_CONTENTS_FPGA,
				FLASH_CONTENTS_PIC,
			} flash_contents;

			uint8_t xbee_channels[2];

			uint8_t robot_number;

			uint8_t dribble_power;
		};

		Property<bool> alive;

		Property<bool> has_feedback;

		Property<bool> ball_in_beam;

		Property<bool> ball_on_dribbler;

		Property<bool> capacitor_charged;

		Property<double> battery_voltage;

		Property<double> capacitor_voltage;

		Property<double> dribbler_temperature;

		Property<unsigned int> break_beam_reading;

		AsyncOperation<void>::Ptr firmware_spi_chip_erase();

		AsyncOperation<void>::Ptr firmware_spi_fill_page_buffer(unsigned int offset, const void *data, std::size_t length);

		AsyncOperation<void>::Ptr firmware_spi_page_program(unsigned int page, uint16_t crc);

		AsyncOperation<uint16_t>::Ptr firmware_spi_block_crc(unsigned int address, std::size_t length);

		AsyncOperation<OperationalParameters>::Ptr firmware_read_operational_parameters();

		AsyncOperation<void>::Ptr firmware_set_operational_parameters(const OperationalParameters &params);

		AsyncOperation<void>::Ptr firmware_commit_operational_parameters();

		AsyncOperation<void>::Ptr firmware_reboot();

		void drive(const int(&wheels)[4]);

		void drive(int w1, int w2, int w3, int w4);

		void drive_scram();

		void dribble(bool active = true);

		void enable_chicker(bool active = true);

		void kick(unsigned int microseconds);

		void test_mode(unsigned int mode);

	private:
		friend class XBeeDongle;

		XBeeDongle &dongle;
		const unsigned int index;
		uint8_t drive_block[10];
		sigc::connection flush_drive_connection;

		static Ptr create(XBeeDongle &dongle, unsigned int index);
		XBeeRobot(XBeeDongle &dongle, unsigned int index);
		~XBeeRobot();
		void flush_drive();
		void on_feedback(const uint8_t *data, std::size_t length);
};

#endif

