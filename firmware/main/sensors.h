#ifndef SENSORS_H
#define SENSORS_H
#include "icb.h"

typedef struct __attribute__((packed)) {
	int16_t x; 
	int16_t y;
	int16_t z;
} sensors_gyro_data_t;

_Static_assert(sizeof(sensors_gyro_data_t) == 6, "Gyro data struct not correct size");

typedef struct __attribute__((packed)) {
	int16_t x;
	int16_t y;
	int16_t z;
} sensors_accel_data_t;

_Static_assert(sizeof(sensors_accel_data_t) == 6, "Accel data struct not correct size");

static sensors_gyro_data_t sensors_get_gyro(void) __attribute__((unused));
static sensors_gyro_data_t sensors_get_gyro(void) {
	sensors_gyro_data_t retval;
	icb_receive(ICB_COMMAND_SENSORS_GET_GYRO, &retval, sizeof(retval));
	return retval;
}

static sensors_accel_data_t sensors_get_accel(void) __attribute__((unused));
static sensors_accel_data_t sensors_get_accel(void) {
	sensors_accel_data_t retval;
	icb_receive(ICB_COMMAND_SENSORS_GET_ACCEL, &retval, sizeof(retval));
	return retval;
}
#endif
