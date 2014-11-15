#ifndef SENSORS_H
#define SENSORS_H

#include "icb.h"
#include <stdint.h>

typedef struct __attribute__((packed)) {
	/**
	 * \brief Whether or not the gyro is working.
	 *
	 * If this has the value 0, there is no gyro data available and the \ref data.failed_chip_id member contains the result of reading the chip ID.
	 * If this has the value 1, there is gyro data available in the \ref data.reading member.
	 */
	uint8_t status;
	/**
	 * \brief The gyro data.
	 */
	union __attribute__((packed)) {
		/**
		 * \brief A successful gyro reading, if the gyro is working.
		 */
		struct __attribute__((packed)) {
			int16_t x; 
			int16_t y;
			int16_t z;
		} reading;
		/**
		 * \brief The result of trying to read the chip ID, if the gyro is not working.
		 */
		uint8_t failed_chip_id;
	} data;
} sensors_gyro_data_t;

_Static_assert(sizeof(sensors_gyro_data_t) == 7U, "Gyro data struct not correct size");

typedef struct __attribute__((packed)) {
	int16_t x;
	int16_t y;
	int16_t z;
} sensors_accel_data_t;

_Static_assert(sizeof(sensors_accel_data_t) == 6U, "Accel data struct not correct size");

inline sensors_gyro_data_t sensors_get_gyro(void) {
	static sensors_gyro_data_t retval;
	icb_receive(ICB_COMMAND_SENSORS_GET_GYRO, &retval, sizeof(retval));
	return retval;
}

inline sensors_accel_data_t sensors_get_accel(void) {
	static sensors_accel_data_t retval;
	icb_receive(ICB_COMMAND_SENSORS_GET_ACCEL, &retval, sizeof(retval));
	return retval;
}

#endif
