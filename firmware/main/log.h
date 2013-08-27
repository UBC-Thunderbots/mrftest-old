#ifndef LOG_H
#define LOG_H

#include <inttypes.h>
#include <stdbool.h>

#define LOG_RECORD_SIZE 128U
#define LOG_MAGIC_TICK UINT32_C(0xE2468842)

/**
 * \brief The type of payload associatd with a tick-type log record.
 */
typedef struct __attribute__((packed)) {
	int16_t breakbeam_diff;
	uint16_t battery_voltage;
	uint16_t capacitor_voltage;
	uint16_t board_temperature;

	float wheels_setpoints[4];
	int16_t wheels_encoder_counts[4];
	int16_t wheels_drives[4];
	uint8_t wheels_temperatures[4];

	uint8_t dribbler_speed;
	uint8_t dribbler_temperature;

	uint8_t encoders_failed;
	uint8_t wheels_hall_sensors_failed;
	uint8_t dribbler_hall_sensors_failed;

	uint32_t cpu_used_since_last_tick;
} log_tick_t;

/**
 * \brief The type of a log record.
 */
typedef struct {
	uint32_t magic;
	uint32_t epoch;
	uint32_t time_msb;
	uint32_t time_lsb;
	union {
		log_tick_t tick;
		uint8_t padding[LOG_RECORD_SIZE - 4 - 4 - 4 - 4];
	};
} log_record_t;

/**
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

	/**
	 * \brief The application tried to write records faster than the card could accept them.
	 *
	 * In this case, a subsequent call to \ref log_alloc may succeed, returning the subsystem to \ref LOG_STATE_OK.
	 */
	LOG_STATE_OVERFLOW,
} log_state_t;

/**
 * \brief Returns the current state of the logging subsystem.
 *
 * \return the current state
 */
log_state_t log_state(void);

/**
 * \brief Initializes the logging subsystem.
 *
 * \pre The SD card must already be initialized.
 *
 * \return \c true on success, or \c false on failure
 */
bool log_init(void);

/**
 * \brief Deinitializes the logging subsystem.
 */
void log_deinit(void);

/**
 * \brief Allocates a log record for the application to write into.
 *
 * The application must finish generating the given record before allocating another record.
 *
 * The newly allocated record will have its epoch and timestamp fields filled in before it is returned.
 * The application must fill the magic field and any record-type-specific data.
 *
 * \return the log record, or null on failure
 */
log_record_t *log_alloc(void);

/**
 * \brief Commits the last allocated log record.
 */
void log_commit(void);

#endif

