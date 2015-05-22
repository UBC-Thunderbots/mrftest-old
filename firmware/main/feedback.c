/**
 * \defgroup FEEDBACK Feedback Packet Functions
 *
 * \brief These functions relate to sending feedback packets over the radio.
 *
 * @{
 */

#include "feedback.h"
#include "adc.h"
#include "breakbeam.h"
#include "charger.h"
#include "dribbler.h"
#include "encoder.h"
#include "hall.h"
#include "icb.h"
#include "log.h"
#include "motor.h"
#include "mrf.h"
#include "priority.h"
#include "sdcard.h"
#include "wheels.h"
#include <FreeRTOS.h>
#include <assert.h>
#include <event_groups.h>
#include <math.h>
#include <unused.h>

#define HAS_BALL_MIN_PERIOD (10U / portTICK_PERIOD_MS)

typedef enum {
	EVENT_SEND_NORMAL = 0x01,
	EVENT_SEND_HAS_BALL = 0x02,
	EVENT_SEND_AUTOKICK = 0x04,
	EVENT_SHUTDOWN = 0x08,
	EVENT_SHUTDOWN_COMPLETE = 0x10,
} event_t;

static EventGroupHandle_t event_group;
static unsigned int has_ball_antispam_ticks = 0U;
static bool has_ball_after_antispam = false;

static void feedback_task(void *UNUSED(param)) {
	static uint8_t nullary_frame[] = {
		9U, // Header length
		9U + 1U, // Total length
		0b01100001, // Frame control LSB (data frame, no security, no frame pending, ack request, intra-PAN)
		0b10001000, // Frame control MSB (16-bit destination address, 16-bit source address)
		0U, // [4] Sequence number
		0U, // [5] PAN ID LSB
		0U, // [6] PAN ID MSB
		0x00U, // Destination address LSB
		0x01U, // Destination address MSB
		0U, // [9] Source address LSB
		0U, // [10] Source address MSB
		0U, // [11] Packet purpose
	};
	for (;;) {
		EventBits_t bits = xEventGroupWaitBits(event_group, EVENT_SEND_NORMAL | EVENT_SEND_HAS_BALL | EVENT_SEND_AUTOKICK | EVENT_SHUTDOWN, pdTRUE, pdFALSE, portMAX_DELAY);

		if (bits & EVENT_SHUTDOWN) {
			xEventGroupSetBits(event_group, EVENT_SHUTDOWN_COMPLETE);
			vTaskDelete(0);
		}
		if (bits & EVENT_SEND_NORMAL) {
			static uint8_t frame[] = {
				9U, // Header length
				9U + 17U, // Total length
				0b01100001, // Frame control LSB (data frame, no security, no frame pending, ack request, intra-PAN)
				0b10001000, // Frame control MSB (16-bit destination address, 16-bit source address)
				0U, // [4] Sequence number
				0U, // [5] PAN ID LSB
				0U, // [6] PAN ID MSB
				0x00U, // Destination address LSB
				0x01U, // Destination address MSB
				0U, // [9] Source address LSB
				0U, // [10] Source address MSB
				0x00U, // Packet purpose (general robot status update)
				0U, 0U, // [12,13] Battery voltage
				0U, 0U, // [14,15] Capacitor voltage
				0U, 0U, // [16,17] Break beam difference
				0U, 0U, // [18,19] Temperature
				0U, // [20] Flags
				0U, // [21] Stuck Hall sensors
				0U, // [22] Additional failures
				0x11U, // [23] SD logger status (both uninitialized)
				0U, 0U, // [24,25] Dribbler speed
				0U, // [26] Hot motors
				0U, // [27] Dribbler temperature
			};

			frame[4U] = mrf_alloc_seqnum();
			uint16_t u16 = mrf_pan_id();
			frame[5U] = u16;
			frame[6U] = u16 >> 8U;
			u16 = mrf_short_address();
			frame[9U] = u16;
			frame[10U] = u16 >> 8U;
			u16 = (uint16_t) (adc_battery() * 1000.0f);
			frame[12U] = u16;
			frame[13U] = u16 >> 8U;
			u16 = (uint16_t) (adc_capacitor() * 100.0f);
			frame[14U] = u16;
			frame[15U] = u16 >> 8U;
			u16 = (uint16_t) (breakbeam_difference() * 1000.0f);
			frame[16U] = u16;
			frame[17U] = u16 >> 8U;
			u16 = (uint16_t) (adc_temperature() * 100.0f);
			frame[18U] = u16;
			frame[19U] = u16 >> 8U;
			uint8_t flags = 0x08U /* Breakout present (undetectable on this board) */ | 0x10U /* Chicker present (undetectable on this board) */;
			if (breakbeam_interrupted()) {
				flags |= 0x01U;
			}
			if (charger_full()) {
				flags |= 0x02U;
			}
			if (charger_timeout()) {
				flags |= 0x04U;
			}
			bool icb_crc_error_reported = icb_crc_error_check();
			if (icb_crc_error_reported) {
				flags |= 0x20U;
			}
			flags |= 0x80U; // For compatibility with pre-2014-08-01 software.
			frame[20U] = flags;
			flags = 0U;
			for (unsigned int i = 0U; i < 4U; ++i) {
				if (motor_hall_stuck_low(i)) {
					flags |= 1U << (i * 2U);
				}
				if (motor_hall_stuck_high(i)) {
					flags |= 1U << (i * 2U + 1U);
				}
			}
			frame[21U] = flags;
			flags = 0U;
			if (motor_hall_stuck_low(4U)) {
				flags |= 1U;
			}
			if (motor_hall_stuck_high(4U)) {
				flags |= 2U;
			}
			for (unsigned int i = 0U; i != 4U; ++i) {
				float hall_rpt = hall_speed(i) / (float) WHEELS_HALL_COUNTS_PER_REV;
				float encoder_rpt = encoder_speed(i) / (float) WHEELS_ENCODER_COUNTS_PER_REV;
				float diff = fabsf(encoder_rpt - hall_rpt);
				// When Hall RPT is x, encoder RPT should be roughly x.
				// However, the extra WHEELS_ENCODER_COUNTS_PER_HALL_COUNT resolution means it is not exactly x.
				// That extra resolution is as much as WHEELS_ENCODER_COUNTS_PER_HALL_COUNT encoder counts.
				// In RPT, that is (WHEELS_ENCODER_COUNTS_PER_HALL_COUNT / WHEELS_ENCODER_COUNTS_PER_REV) RPT.
				// Double that, and ignore any errors with absolute magnitude less than that threshold.
				//
				// Also, to allow for physical imperfections in the sensors, also ignore any error of less than 10%.
				if (diff > 2.0f * WHEELS_ENCODER_COUNTS_PER_HALL_COUNT / WHEELS_ENCODER_COUNTS_PER_REV && diff > 0.1f * fabsf(hall_rpt)) {
					flags |= 1U << (i + 2U);
				}
			}
			if (mrf_receive_fcs_fail_check_clear()) {
				flags |= 1U << 6U;
			}
			frame[22U] = flags;
//#warning SD status has more than 16 possible errors!
//			frame[23U] = (((unsigned int) log_state()) << 4U) | (((unsigned int) sd_status()) & 0x0FU);
			frame[23U] = log_state() << 4;
#warning TODO more detailed SD status reporting
			int16_t i16 = hall_speed(4U);
			frame[24U] = (uint8_t) (uint16_t) i16;
			frame[25U] = (uint8_t) (((uint16_t) i16) >> 8U);
			flags = wheels_hot();
			if (dribbler_hot()) {
				flags |= 0x10U;
			}
			frame[26U] = flags;
			{
				unsigned int temp = dribbler_temperature();
				if (temp > 255U) {
					temp = 255U;
				}
				frame[27U] = (uint8_t) temp;
			}
			mrf_tx_result_t result = mrf_transmit(frame);
			if (result == MRF_TX_OK) {
				motor_hall_stuck_clear();
				if (icb_crc_error_reported) {
					icb_crc_error_clear();
				}
				// We no longer need to send a has-ball update, because the information it would convey is in the feedback packet.
				bits &= ~EVENT_SEND_HAS_BALL;
			}
		}
		if (bits & EVENT_SEND_HAS_BALL) {
			nullary_frame[4U] = mrf_alloc_seqnum();
			uint16_t u16 = mrf_pan_id();
			nullary_frame[5U] = u16;
			nullary_frame[6U] = u16 >> 8U;
			u16 = mrf_short_address();
			nullary_frame[9U] = u16;
			nullary_frame[10U] = u16 >> 8U;
			nullary_frame[11U] = breakbeam_interrupted() ? 0x04U : 0x05U;
			// No need to check for failure.
			// If the frame is not delivered, the next feedback packet will give the host fully up-to-date information.
			mrf_transmit(nullary_frame);
		}
		if (bits & EVENT_SEND_AUTOKICK) {
			nullary_frame[4U] = mrf_alloc_seqnum();
			uint16_t u16 = mrf_pan_id();
			nullary_frame[5U] = u16;
			nullary_frame[6U] = u16 >> 8U;
			u16 = mrf_short_address();
			nullary_frame[9U] = u16;
			nullary_frame[10U] = u16 >> 8U;
			nullary_frame[11U] = 0x01U;
			if (mrf_transmit(nullary_frame) != MRF_TX_OK) {
				// Delivery failed.
				// This message absolutely must go through.
				// If not, the host may believe autokick is still armed even though it isn’t!
				// So, try delivering the message again later.
				xEventGroupSetBits(event_group, EVENT_SEND_AUTOKICK);
			}
		}
	}
}

/**
 * \brief Initializes the feedback system.
 */
void feedback_init(void) {
	event_group = xEventGroupCreate();
	assert(event_group);
	BaseType_t ok = xTaskCreate(&feedback_task, "feedback", 512U, 0, PRIO_TASK_FEEDBACK, 0);
	assert(ok == pdPASS);
}

/**
 * \brief Shuts down the feedback system.
 */
void feedback_shutdown(void) {
	xEventGroupSetBits(event_group, EVENT_SHUTDOWN);
	xEventGroupWaitBits(event_group, EVENT_SHUTDOWN_COMPLETE, pdFALSE, pdFALSE, portMAX_DELAY);
}

/**
 * \brief Marks normal feedback as pending.
 *
 * The feedback task will send feedback as soon as possible after this function is called.
 */
void feedback_pend_normal(void) {
	xEventGroupSetBits(event_group, EVENT_SEND_NORMAL);
}

/**
 * \brief Marks has-ball feedback as pending.
 *
 * The feedback task will send a has-ball message as soon as possible after this function is called.
 */
void feedback_pend_has_ball(void) {
	taskENTER_CRITICAL();
	if (has_ball_antispam_ticks) {
		has_ball_after_antispam = true;
	} else {
		xEventGroupSetBits(event_group, EVENT_SEND_HAS_BALL);
		has_ball_antispam_ticks = HAS_BALL_MIN_PERIOD;
	}
	taskEXIT_CRITICAL();
}

/**
 * \brief Marks autokick feedback as pending.
 *
 * The feedback task will send an autokick fired message as soon as possible after this function is called.
 */
void feedback_pend_autokick(void) {
	xEventGroupSetBits(event_group, EVENT_SEND_AUTOKICK);
}

/**
 * \brief Ticks the feedback module.
 */
void feedback_tick(void) {
	taskENTER_CRITICAL();
	if (has_ball_antispam_ticks) {
		--has_ball_antispam_ticks;
	}
	if (!has_ball_antispam_ticks && has_ball_after_antispam) {
		xEventGroupSetBits(event_group, EVENT_SEND_HAS_BALL);
		has_ball_after_antispam = false;
	}
	taskEXIT_CRITICAL();
}

/**
 * @}
 */

