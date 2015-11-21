/**
 * \defgroup RECEIVE Receive Functions
 *
 * \brief These functions handle receiving radio packets and acting on them.
 *
 * For drive packets, each received packet is decoded, and the most recent data is made available for the tick functions elsewhere to act on.
 * Also, a drive packet that requests feedback immediately notifies the feedback module.
 *
 * For message packets, the action required by the packet is taken immediately.
 *
 * \{
 */

#include "receive.h"
#include "charger.h"
#include "chicker.h"
#include "dma.h"
#include "feedback.h"
#include "leds.h"
#include "main.h"
#include "motor.h"
#include "mrf.h"
#include "priority.h"
#include "rtc.h"
#include "primitives/primitive.h"
#include <FreeRTOS.h>
#include <assert.h>
#include <semphr.h>
#include <stack.h>
#include <task.h>
#include <unused.h>

/**
 * \brief The number of robots in the drive packet.
 */
#define RECEIVE_DRIVE_NUM_ROBOTS 8

/**
 * \brief The number of bytes of drive packet data per robot.
 */
#define RECEIVE_DRIVE_BYTES_PER_ROBOT 9

static unsigned int robot_index;
static uint8_t *dma_buffer;
static SemaphoreHandle_t drive_mtx;
static unsigned int timeout_ticks;
static uint8_t last_serial = 0xFF;
STACK_ALLOCATE(receive_task_stack, 4096);

static void receive_task(void *UNUSED(param)) {
	uint16_t last_sequence_number = 0xFFFFU;
	bool last_estop_run = false;
	size_t frame_length;

	while ((frame_length = mrf_receive(dma_buffer))) {
		uint16_t frame_control = dma_buffer[0U] | (dma_buffer[1U] << 8U);
		// Sanity-check the frame control word
		if (((frame_control >> 0U) & 7U) == 1U /* Data packet */ && ((frame_control >> 3U) & 1U) == 0U /* No security */ && ((frame_control >> 6U) & 1U) == 1U /* Intra-PAN */ && ((frame_control >> 10U) & 3U) == 2U /* 16-bit destination address */ && ((frame_control >> 14U) & 3U) == 2U /* 16-bit source address */) {
			// Read out and check the source address and sequence number
			uint16_t source_address = dma_buffer[7U] | (dma_buffer[8U] << 8U);
			uint8_t sequence_number = dma_buffer[2U];
			if (source_address == 0x0100U && sequence_number != last_sequence_number) {
				// Update sequence number
				last_sequence_number = sequence_number;

				// Handle packet
				uint16_t dest_address = dma_buffer[5U] | (dma_buffer[6U] << 8U);
				static const size_t HEADER_LENGTH = 2U /* Frame control */ + 1U /* Seq# */ + 2U /* Dest PAN */ + 2U /* Dest */ + 2U /* Src */;
				static const size_t FOOTER_LENGTH = 2U /* FCS */ + 1U /* RSSI */ + 1U /* LQI */;
				if (dest_address == 0xFFFFU) {
					// Broadcast frame must contain a drive packet, which must
					// contain:
					// - 8×8=64 bytes of robot drive data
					// - 1 byte of serial number
					// - 1 byte of emergency stop condition
					// - 8 bytes of timestamp
					static const size_t BODY_LENGTH = RECEIVE_DRIVE_NUM_ROBOTS * RECEIVE_DRIVE_BYTES_PER_ROBOT + 1 + 8;
					if (frame_length == HEADER_LENGTH + BODY_LENGTH + FOOTER_LENGTH) {
						// Grab emergency stop status and timestamp from the
						// end of the frame.
						bool estop_run = !!dma_buffer[HEADER_LENGTH + RECEIVE_DRIVE_NUM_ROBOTS * RECEIVE_DRIVE_BYTES_PER_ROBOT];
						uint64_t timestamp = 0;
						for (unsigned int i = 7; i < 8; --i) {
							timestamp <<= 8;
							timestamp |= dma_buffer[HEADER_LENGTH + RECEIVE_DRIVE_NUM_ROBOTS * RECEIVE_DRIVE_BYTES_PER_ROBOT + 1 + i];
						}
						rtc_set(timestamp);

						// Grab a pointer to the robot’s own data block.
						const uint8_t *robot_data = dma_buffer + HEADER_LENGTH + RECEIVE_DRIVE_BYTES_PER_ROBOT * robot_index;

						// Check if feedback should be sent.
						if (robot_data[0] & 0x80) {
							feedback_pend_normal();
						}

						// Extract the serial number.
						uint8_t serial = *robot_data++ & 0x0F;

						// Construct the individual 16-bit words sent from the host.
						uint16_t words[4U];
						for (unsigned int i = 0U; i < 4U; ++i) {
							words[i] = *robot_data++;
							words[i] |= (uint16_t)*robot_data++ << 8;
						}

						// In case of emergency stop, treat everything as zero
						// except the chicker discharge bit (that can keep its
						// status).
						if (!estop_run) {
							static const uint16_t MASK[4] = { 0x0000, 0x4000, 0x0000, 0x0000 };
							for (unsigned int i = 0; i != 4; ++i) {
								words[i] &= MASK[i];
							}
						}

						// Take the drive mutex.
						xSemaphoreTake(drive_mtx, portMAX_DELAY);

						// Reset timeout.
						timeout_ticks = 1000U / portTICK_PERIOD_MS;

						// Apply the charge and discharge mode.
						charger_enable(words[1] & 0x8000);
						chicker_discharge(words[1] & 0x4000);

						// If the serial number, the emergency stop has just
						// been switched to stop, or the current primitive is
						// direct, a new movement primitive needs to start. Do
						// not start a movement primitive if the emergency stop
						// has just been switched to run and the current
						// primitive is not direct, because we can’t usefully
						// restart the stopped prior primitive—instead, wait
						// for the host to send new data. Strictly speaking, we
						// only need to start direct primitives when the estop
						// is switched from stop to run, but this is
						// unnecessary as direct primitives shouldn’t care
						// about being started more often than necessary.
						unsigned int primitive;
						primitive_params_t pparams;
						for (unsigned int i = 0; i != 4; ++i) {
							int16_t value = words[i] & 0x3FF;
							if (words[i] & 0x400) {
								value = -value;
							}
							if (words[i] & 0x800) {
								value *= 10;
							}
							pparams.params[i] = value;
						}
						primitive = words[0] >> 12;
						pparams.extra = (words[2] >> 12) | ((words[3] >> 12) << 4);
						pparams.slow = !!(pparams.extra & 0x80);
						pparams.extra &= 0x7F;
						if ((serial != last_serial /* Non-atomic because we are only writer */) || (!estop_run && last_estop_run) || primitive_is_direct(primitive)) {
							// Apply the movement primitive.
							primitive_start(primitive, &pparams);
						}

						// Release the drive mutex.
						xSemaphoreGive(drive_mtx);

						// Update the last values.
						__atomic_store_n(&last_serial, serial, __ATOMIC_RELAXED);
						last_estop_run = estop_run;
					}
				} else if (frame_length >= HEADER_LENGTH + 1U + FOOTER_LENGTH) {
					// Non-broadcast frame contains a message specifically for this robot
					static const uint16_t MESSAGE_PURPOSE_ADDR = HEADER_LENGTH;
					static const uint16_t MESSAGE_PAYLOAD_ADDR = MESSAGE_PURPOSE_ADDR + 1U;
					switch (dma_buffer[MESSAGE_PURPOSE_ADDR]) {
						case 0x00U: // Fire kicker immediately
							if (frame_length == HEADER_LENGTH + 4U + FOOTER_LENGTH) {
								uint8_t which = dma_buffer[MESSAGE_PAYLOAD_ADDR];
								uint16_t width = dma_buffer[MESSAGE_PAYLOAD_ADDR + 2U];
								width <<= 8U;
								width |= dma_buffer[MESSAGE_PAYLOAD_ADDR + 1U];
								chicker_fire(which ? CHICKER_CHIP : CHICKER_KICK, width);
							}
							break;

						case 0x01U: // Arm autokick
							if (frame_length == HEADER_LENGTH + 4U + FOOTER_LENGTH) {
								uint8_t which = dma_buffer[MESSAGE_PAYLOAD_ADDR];
								uint16_t width = dma_buffer[MESSAGE_PAYLOAD_ADDR + 2U];
								width <<= 8U;
								width |= dma_buffer[MESSAGE_PAYLOAD_ADDR + 1U];
								chicker_auto_arm(which ? CHICKER_CHIP : CHICKER_KICK, width);
							}
							break;

						case 0x02U: // Disarm autokick
							if (frame_length == HEADER_LENGTH + 1U + FOOTER_LENGTH) {
								chicker_auto_disarm();
							}
							break;

						case 0x03U: // Set LED mode
							if (frame_length == HEADER_LENGTH + 2U + FOOTER_LENGTH) {
								uint8_t mode = dma_buffer[MESSAGE_PAYLOAD_ADDR];
								if (mode <= 4U) {
									leds_test_set_mode(LEDS_TEST_MODE_HALL, mode);
								} else if (5U <= mode && mode <= 8U) {
									leds_test_set_mode(LEDS_TEST_MODE_ENCODER, mode - 5U);
								} else if (mode == 0x21U) {
									leds_test_set_mode(LEDS_TEST_MODE_CONSTANT, 0x7U);
								} else {
									leds_test_set_mode(LEDS_TEST_MODE_NORMAL, 0U);
								}
							}
							break;

						case 0x08U: // Reboot
							if (frame_length == HEADER_LENGTH + 1U + FOOTER_LENGTH) {
								main_shutdown(MAIN_SHUT_MODE_REBOOT);
							}
							break;

						case 0x09U: // Force on motor power
							if (frame_length == HEADER_LENGTH + 1U + FOOTER_LENGTH) {
								motor_force_power();
							}
							break;

						case 0x0CU: // Shut down
							if (frame_length == HEADER_LENGTH + 1U + FOOTER_LENGTH) {
								main_shutdown(MAIN_SHUT_MODE_POWER);
							}
							break;

						case 0x0DU: // Request build IDs
							feedback_pend_build_ids();
							break;
					}
				}
			}
		}
	}

	// mrf_receive returned zero, which means a cancellation has been requested.
	// This means we are shutting down.
	xSemaphoreGive(main_shutdown_sem);
	vTaskSuspend(0);
}

/**
 * \brief Initializes the receive task.
 *
 * \param[in] index the robot index
 */
void receive_init(unsigned int index) {
	drive_mtx = xSemaphoreCreateMutex();
	assert(drive_mtx);

	robot_index = index;

	dma_memory_handle_t dma_buffer_handle = dma_alloc(128U);
	assert(dma_buffer_handle);
	dma_buffer = dma_get_buffer(dma_buffer_handle);

	BaseType_t ok = xTaskGenericCreate(&receive_task, "rx", sizeof(receive_task_stack) / sizeof(*receive_task_stack), 0, PRIO_TASK_RX, 0, receive_task_stack, 0);
	assert(ok == pdPASS);
}

/**
 * \brief Stops the receive task.
 */
void receive_shutdown(void) {
	mrf_receive_cancel();
	xSemaphoreTake(main_shutdown_sem, portMAX_DELAY);
}

/**
 * \brief Ticks the receiver.
 */
void receive_tick(void) {
	// Decrement timeout tick counter if nonzero.
	xSemaphoreTake(drive_mtx, portMAX_DELAY);
	if (timeout_ticks == 1) {
		timeout_ticks = 0;
		charger_enable(false);
		chicker_discharge(true);
		static const primitive_params_t ZERO_PARAMS;
		primitive_start(0, &ZERO_PARAMS);
	} else if (timeout_ticks > 1) {
		--timeout_ticks;
	}
	xSemaphoreGive(drive_mtx);
}

/**
 * \brief Returns the serial number of the most recent drive packet.
 */
uint8_t receive_last_serial(void) {
	return __atomic_load_n(&last_serial, __ATOMIC_RELAXED);
}

/**
 * \}
 */
