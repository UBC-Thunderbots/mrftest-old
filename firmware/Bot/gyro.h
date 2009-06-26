#ifndef GYRO_H
#define GYRO_H

/*
 * Zeroes the gyroscope.
 */
void gyro_init(void);

/*
 * Reads the gyroscope.
 */
double gyro_read(void);

/*
 * Checks whether a gyroscope is present.
 */
uint8_t gyro_present(void);

#endif

