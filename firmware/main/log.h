#ifndef LOG_H
#define LOG_H

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>

#define LOG_RECORD_SIZE 512U
#define LOG_MAGIC_TICK UINT32_C(0xE2468843)

/**
 * \ingroup LOG
 *
 * \brief The type of payload associated with a tick-type log record.
 */
typedef struct __attribute__((packed)) {
	float breakbeam_diff;
	float battery_voltage;
	float capacitor_voltage;

	int16_t wheels_setpoints[4U];
	int16_t wheels_encoder_counts[4U];
	int16_t wheels_drives[4U];
	uint8_t wheels_temperatures[4U];
	uint8_t wheels_encoders_failed;
	uint8_t wheels_hall_sensors_failed;

	uint8_t dribbler_ticked;
	uint8_t dribbler_pwm;
	uint8_t dribbler_speed;
	uint8_t dribbler_temperature;
	uint8_t dribbler_hall_sensors_failed;
} log_tick_t;

/**
 * \ingroup LOG
 *
 * \brief The type of a log record.
 */
typedef struct {
	uint32_t magic;
	uint32_t epoch;
	uint32_t time;
	union {
		log_tick_t tick;
		uint8_t padding[LOG_RECORD_SIZE - 4U - 4U - 4U];
	};
} log_record_t;

/**
 * \ingroup LOG
 *
 * \brief The possible states the logging subsystem can be in.
 */
typedef enum {
	/**
	 * \brief The logging subsystem is working properly.
	 */
	LOG_STATE_OK,

	/**
	 * \brief The logging subsystem has not yet been initialized or has been deinitialized.
	 */
	LOG_STATE_UNINITIALIZED,

	/**
	 * \brief The logging subsystem has shut down permanently because the SD card module reported an error.
	 */
	LOG_STATE_SD_ERROR,

	/**
	 * \brief The logging subsystem has shut down permanently because the SD card is full.
	 */
	LOG_STATE_CARD_FULL,
} log_state_t;

log_state_t log_state(void);
bool log_init(void);
void log_shutdown(void);
log_record_t *log_alloc(void);
void log_queue(log_record_t *record);

#endif

