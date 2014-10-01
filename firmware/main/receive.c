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
 * @{
 */

#include "receive.h"
#include "chicker.h"
#include "dma.h"
#include "drive.h"
#include "feedback.h"
#include "leds.h"
#include "main.h"
#include "motor.h"
#include "mrf.h"
#include "priority.h"
#include <FreeRTOS.h>
#include <assert.h>
#include <semphr.h>
#include <task.h>
#include <unused.h>

static unsigned int robot_index;
static uint8_t *dma_buffer;
static SemaphoreHandle_t shutdown_sem;
static unsigned int timeout_ticks;

static void receive_task(void *UNUSED(param)) {
	uint16_t last_sequence_number = 0xFFFFU;
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
					// Broadcast frame must contain drive packet, which must be 65 bytes long
					if (frame_length == HEADER_LENGTH + 65U + FOOTER_LENGTH) {
						// Construct the individual 16-bit words sent from the host.
						const uint8_t offset = HEADER_LENGTH + 8U * robot_index;
						uint16_t words[4U];
						for (unsigned int i = 0U; i < 4U; ++i) {
							words[i] = dma_buffer[offset + i * 2U + 1U];
							words[i] <<= 8U;
							words[i] |= dma_buffer[offset + i * 2U];
						}

						// Pull out the serial number at the end.
						uint8_t drive_data_serial = dma_buffer[HEADER_LENGTH + 64U];

						// Check for feedback request.
						if (!!(words[0U] & 0x8000U)) {
							feedback_pend_normal();
						}

						// Find the packet buffer to write into.
						drive_t *target = drive_write_get();

						// Decode the drive packet.
						switch ((words[0U] >> 13U) & 0b11) {
							case 0b00: target->wheels_mode = WHEELS_MODE_COAST; break;
							case 0b01: target->wheels_mode = WHEELS_MODE_BRAKE; break;
							case 0b10: target->wheels_mode = WHEELS_MODE_OPEN_LOOP; break;
							case 0b11: target->wheels_mode = WHEELS_MODE_CLOSED_LOOP; break;
						}
						target->dribbler_power = 255U * (words[2U] >> 11U) / 31U;
						target->charger_enabled = !!(words[1U] & (1U << 15U));
						target->discharger_enabled = !!(words[1U] & (1U << 14U));
						for (unsigned int i = 0U; i < 4U; ++i) {
							int16_t sp = (int16_t) (words[i] & 0x3FFU);
							if (words[i] & 0x400U) {
								sp = -sp;
							}
							target->setpoints[i] = sp;
						}
						target->data_serial = drive_data_serial;

						// Put the written buffer back.
						drive_write_put();
						target = 0;

						// Reset timeout.
						__atomic_store_n(&timeout_ticks, 1000U / portTICK_PERIOD_MS, __ATOMIC_RELAXED);
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
								if (0U <= mode && mode <= 4U) {
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
								main_shutdown(true);
							}
							break;

						case 0x09U: // Force on motor power
							if (frame_length == HEADER_LENGTH + 1U + FOOTER_LENGTH) {
								motor_force_power();
							}
							break;

						case 0x0CU: // Shut down
							if (frame_length == HEADER_LENGTH + 1U + FOOTER_LENGTH) {
								main_shutdown(false);
							}
							break;
					}
				}
			}
		}
	}

	// mrf_receive returned zero, which means a cancellation has been requested.
	// This means we are shutting down.
	xSemaphoreGive(shutdown_sem);
	vTaskDelete(0);
}

/**
 * \brief Initializes the receive task.
 *
 * \param[in] index the robot index
 */
void receive_init(unsigned int index) {
	robot_index = index;

	dma_memory_handle_t dma_buffer_handle = dma_alloc(128U);
	assert(dma_buffer_handle);
	dma_buffer = dma_get_buffer(dma_buffer_handle);

	BaseType_t ok = xTaskCreate(&receive_task, "rx", 512U, 0, PRIO_TASK_RX, 0);
	assert(ok == pdPASS);
}

/**
 * \brief Stops the receive task.
 */
void receive_shutdown(void) {
	shutdown_sem = xSemaphoreCreateBinary();
	assert(shutdown_sem);
	mrf_receive_cancel();
	xSemaphoreTake(shutdown_sem, portMAX_DELAY);
	vSemaphoreDelete(shutdown_sem);
}

/**
 * \brief Ticks the receiver.
 */
void receive_tick(void) {
	// Decrement timeout tick counter if nonzero.
	unsigned int old, new;
	do {
		old = __atomic_load_n(&timeout_ticks, __ATOMIC_RELAXED);
		new = old ? old - 1U : old;
	} while (!__atomic_compare_exchange_n(&timeout_ticks, &old, new, false, __ATOMIC_RELAXED, __ATOMIC_RELAXED));
}

/**
 * \brief Checks whether there has been a receive timeout indicating that the host is no longer sending orders.
 *
 * \retval true if a timeout has occurred
 * \retval false if a timeout has not occurred
 */
bool receive_drive_packet_timeout(void) {
	return !__atomic_load_n(&timeout_ticks, __ATOMIC_RELAXED);
}

/**
 * @}
 */

