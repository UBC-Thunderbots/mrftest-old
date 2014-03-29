#include "normal.h"
#include "constants.h"
#include "estop.h"
#include "mrf.h"
#include "radio_config.h"
#include <FreeRTOS.h>
#include <errno.h>
#include <event_groups.h>
#include <gpio.h>
#include <queue.h>
#include <rcc.h>
#include <semphr.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <unused.h>
#include <usb.h>
#include <registers/exti.h>
#include <registers/timer.h>

/**
 * \brief The exact number of bytes in the payload of a drive packet.
 *
 * This number does not include the data counter.
 */
#define DRIVE_PACKET_DATA_SIZE 64U

/**
 * \brief The number of packet buffers to allocate at system startup.
 */
#define NUM_PACKETS 256U

enum {
	/**
	 * \brief The event bit asserted for the drive packet task when a timer interval expires.
	 */
	DRIVE_EVENT_TICK = 0x01,

	/**
	 * \brief The event bit asserted for the drive packet task when a transfer completes on the USB endpoint.
	 */
	DRIVE_EVENT_ENDPOINT = 0x02,
};

/**
 * \brief A packet buffer.
 */
typedef struct {
	/**
	 * \brief The message ID, used for outbound reliable packets to report delivery status.
	 */
	uint8_t message_id;

	/**
	 * \brief For outbound packets, whether a message delivery report should be returned.
	 */
	bool reliable;

	/**
	 * \brief The position of the data within the data array.
	 */
	uint8_t data_offset;

	/**
	 * \brief The length of the data.
	 */
	uint8_t length;

	/**
	 * \brief The buffer which contains the data.
	 */
	uint8_t data[128U];
} packet_t;

/**
 * \brief A message delivery report, exactly as delivered over the USB endpoint.
 */
typedef struct __attribute__((packed)) {
	/**
	 * \brief The message ID from the original packet.
	 */
	uint8_t message_id;

	/**
	 * \brief The delivery status.
	 */
	uint8_t status;
} mdr_t;

/**
 * \brief The last 802.15.4 sequence number used in a transmitted packet.
 */
static uint8_t mrf_tx_seqnum;

/**
 * \brief The index of the next robot to poll for status updates.
 */
static unsigned int poll_index;

/**
 * \brief Whether or not the radio receive task is shutting down.
 */
static bool rdrx_shutting_down;

/**
 * \brief A queue of pointers to free packet buffers available for use.
 */
static QueueHandle_t free_queue;

/**
 * \brief A queue of pointers to packet buffers for messages waiting to send over the radio.
 *
 * A null pointer is pushed into this queue during shutdown, signalling the radio task to terminate.
 */
static QueueHandle_t transmit_queue;

/**
 * \brief A queue of pointers to packet buffers for messages waiting to send over USB.
 *
 * A null pointer is pushed into this queue during shutdown, signalling the receive message USB task to terminate.
 */
static QueueHandle_t receive_queue;

/**
 * \brief A queue of message delivery report structures waiting to send over USB.
 *
 * A structure with both elements set to 0xFF is pushed into this queue during shutdown, signalling the MDR USB task to terminate.
 */
static QueueHandle_t mdr_queue;

/**
 * \brief A semaphore given whenever the hardware run switch changes state.
 */
static SemaphoreHandle_t estop_sem;

/**
 * \brief A semaphore given whenever EXTI12 fires.
 */
static SemaphoreHandle_t mrf_int_sem;

/**
 * \brief A mutex that must be held whenever a task is using the radio to transmit a frame.
 */
static SemaphoreHandle_t transmit_mutex;

/**
 * \brief A semaphore given whenever the current transmit operation completes.
 */
static SemaphoreHandle_t transmit_complete_sem;

/**
 * \brief A semaphore that is given every 20 ms, when a drive packet should be transmitted.
 */
static EventGroupHandle_t drive_event_group;

/**
 * \brief A semaphore used to signal task shutdown.
 */
static SemaphoreHandle_t shutdown_sem;

/**
 * \brief Handles rising edge interrupts on the MRF interrupt line.
 */
static void mrf_int_isr(void) {
	// Notify the task.
	BaseType_t yield = pdFALSE;
	xSemaphoreGiveFromISR(mrf_int_sem, &yield);
	if (yield) {
		portYIELD_FROM_ISR();
	}
}

/**
 * \brief Handles timer 6 interrupts.
 */
void timer6_isr(void) {
	// Clear interrupt flag.
	{
		TIM_basic_SR_t tmp = { 0 };
		TIM6.SR = tmp;
	}

	// Notify task.
	BaseType_t yield = pdFALSE;
	xEventGroupSetBitsFromISR(drive_event_group, DRIVE_EVENT_TICK, &yield);
	if (yield) {
		portYIELD_FROM_ISR();
	}
}

/**
 * \brief Writes a drive packet into the radio transmit buffer and begins sending it.
 *
 * This function also blinks the transmit LED.
 *
 * \param[in] packet the 64-byte drive packet
 * \param[in] counter the 1-byte data counter, incremented when new data arrives
 *
 * \pre The transmit mutex must be held by the caller.
 */
static void send_drive_packet(const void *packet, uint8_t counter) {
	// Write out the packet.
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 0U, 9U); // Header length
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 1U, 9U + DRIVE_PACKET_DATA_SIZE + 1U); // Frame length
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 2U, 0b01000001U); // Frame control LSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 3U, 0b10001000U); // Frame control MSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 4U, ++mrf_tx_seqnum); // Sequence number
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 5U, radio_config.pan_id); // Destination PAN ID LSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 6U, radio_config.pan_id >> 8U); // Destination PAN ID MSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 7U, 0xFFU); // Destination address LSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 8U, 0xFFU); // Destination address MSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 9U, 0x00U); // Source address LSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 10U, 0x01U); // Source address MSB
	const uint8_t *bptr = packet;
	if (estop_read() != ESTOP_RUN) {
		for (size_t i = 0U; i < DRIVE_PACKET_DATA_SIZE; i += 8U) {
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 11U + i + 1U, (i == poll_index * DRIVE_PACKET_DATA_SIZE / 8U) ? 0b10000000U : 0b00000000U);
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 11U + i + 0U, 0x00U);
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 11U + i + 3U, 0b01000000U);
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 11U + i + 2U, 0x00U);
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 11U + i + 5U, 0x00U);
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 11U + i + 4U, 0x00U);
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 11U + i + 7U, 0x00U);
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 11U + i + 6U, 0x00U);
		}
	} else {
		for (size_t i = 0U; i < DRIVE_PACKET_DATA_SIZE; ++i) {
			uint8_t mask = 0U;
			if (i - 1U == poll_index * DRIVE_PACKET_DATA_SIZE / 8U) {
				mask = 0x80U;
			}
			mrf_write_long(MRF_REG_LONG_TXNFIFO + 11U + i, bptr[i] | mask);
		}
	}
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 11U + DRIVE_PACKET_DATA_SIZE, counter);
	poll_index = (poll_index + 1U) % 8U;

	// Initiate transmission with no acknowledgement.
	mrf_write_short(MRF_REG_SHORT_TXNCON, 0b00000001U);

	// Blink the transmit light.
	gpio_toggle(GPIOB, 13U);
}

/**
 * \brief Handles all work associated with drive packets.
 *
 * This task both runs OUT endpoint 1 (in asynchronous mode) and also runs the radio while transmitting drive packets.
 * Double-buffering is used so that the USB endpoint can be enabled without inhibiting radio operation.
 *
 * The drive packet is sent over the radio every time timer 6 expires, as long as the packet is valid.
 * On receiving a packet over USB, the new packet is used on the next scheduled transmission and thereafter.
 */
static void drive_task(void *UNUSED(param)) {
	uint8_t *packet_buffers[2U];
	uint8_t counter = 0U;
	unsigned int wptr = 0U, rptr = 0U;

	// Allocate packet buffers.
	for (unsigned int i = 0U; i != 2U; ++i) {
		packet_buffers[i] = malloc(DRIVE_PACKET_DATA_SIZE);
		assert(packet_buffers[i]);
	}

	// Set up timer 6 to overflow every 20 milliseconds for the drive packet.
	// Timer 6 input is 72 MHz from the APB.
	// Need to count to 1,440,000 for each overflow.
	// Set prescaler to 1,000, auto-reload to 1,440.
	rcc_enable_reset(APB1, TIM6);
	{
		TIM_basic_CR1_t tmp = {
			.ARPE = 0, // ARR is not buffered.
			.OPM = 0, // Counter counters forever.
			.URS = 1, // Update interrupts and DMA requests generated only at counter overflow.
			.UDIS = 0, // Updates not inhibited.
			.CEN = 0, // Timer not currently enabled.
		};
		TIM6.CR1 = tmp;
	}
	{
		TIM_basic_DIER_t tmp = {
			.UDE = 0, // DMA disabled.
			.UIE = 1, // Interrupt enabled.
		};
		TIM6.DIER = tmp;
	}
	TIM6.PSC = 999U;
	TIM6.ARR = 1439U;
	TIM6.CNT = 0U;
	TIM6.CR1.CEN = 1; // Enable timer
	portENABLE_HW_INTERRUPT(54U, EXCEPTION_MKPRIO(6U, 0U));

	// Run!
	bool ep_running = false;
	for (;;) {
		// Start the endpoint if possible.
		if (!ep_running) {
			if (uep_async_read_start(0x01U, packet_buffers[wptr], DRIVE_PACKET_DATA_SIZE, drive_event_group, DRIVE_EVENT_ENDPOINT)) {
				ep_running = true;
			} else {
				if (errno == EPIPE) {
					// Endpoint halted.
					rptr = wptr = 0U;
					uep_halt_wait(0x01U);
				} else {
					// Shutting down.
					break;
				}
			}
		}

		// Wait for activity.
		EventBits_t bits = xEventGroupWaitBits(drive_event_group, DRIVE_EVENT_TICK | DRIVE_EVENT_ENDPOINT, pdTRUE, pdFALSE, portMAX_DELAY);

		if (bits & DRIVE_EVENT_ENDPOINT) {
			// Endpoint finished.
			ep_running = false;
			size_t transfer_length;
			if (uep_async_read_finish(0x01U, &transfer_length)) {
				ep_running = false;
				bool ok = false;
				if (transfer_length == DRIVE_PACKET_DATA_SIZE) {
					// Check for illegal bit 15 being set.
					ok = true;
					for (unsigned int robot = 0U; robot != 8U; ++robot) {
						const uint8_t *bot_data = &packet_buffers[wptr][robot * DRIVE_PACKET_DATA_SIZE / 8U];
						uint16_t first_word = bot_data[0U] | ((uint16_t) bot_data[1U] << 8U);
						if (first_word & 0x8000U) {
							ok = false;
						}
					}
				} else {
					// Transfer is wrong length; reject.
					ok = false;
				}
				if (ok) {
					// Transfer OK; accept this packet and use it on next tick.
					rptr = wptr;
					wptr = !wptr;
					++counter;
				} else {
					// Halt endpoint due to application being dumb.
					uep_halt(0x01U);
				}
			} else if (errno == ECONNRESET) {
				// Shutting down.
				ep_running = false;
				break;
			} else if (errno == EOVERFLOW) {
				// Halt endpoint due to application being dumb.
				uep_halt(0x01U);
			} else if (errno != EINPROGRESS) {
				ep_running = false;
			}
		}

		if (bits & DRIVE_EVENT_TICK) {
			if (rptr != wptr) {
				// Send a packet.
				xSemaphoreTake(transmit_mutex, portMAX_DELAY);
				send_drive_packet(packet_buffers[rptr], counter);
				xSemaphoreTake(transmit_complete_sem, portMAX_DELAY);
				xSemaphoreGive(transmit_mutex);
			}
		}
	}

	// Turn off timer 6.
	{
		TIM_basic_CR1_t tmp = { 0 };
		TIM6.CR1 = tmp; // Disable counter
	}
	portDISABLE_HW_INTERRUPT(54U);
	rcc_disable(APB1, TIM6);

	// Free packet buffers.
	for (unsigned int i = 0U; i != 2U; ++i) {
		free(packet_buffers[i]);
	}

	// Done.
	xSemaphoreGive(shutdown_sem);
	vTaskDelete(0);
}

/**
 * \brief Receives reliable message packets from OUT endpoint 2 and queues them for transmission.
 */
static void reliable_task(void *UNUSED(param)) {
	// Run.
	packet_t *buf = 0;
	for (;;) {
		if (!buf) {
			xQueueReceive(free_queue, &buf, portMAX_DELAY);
		}
		size_t length;
		if (uep_read(0x02U, buf->data, sizeof(buf->data), &length)) {
			if (length >= 2U && ((buf->data[0U] & 0x0FU) < 8U)) {
				buf->message_id = buf->data[1U];
				buf->reliable = true;
				buf->data_offset = 2U;
				buf->length = length - 2U;
				xQueueSend(transmit_queue, &buf, portMAX_DELAY);
				buf = 0;
			} else {
				// Halt endpoint due to application being dumb.
				uep_halt(0x02U);
			}
		} else if (errno == EPIPE) {
			// Halted.
			if (!uep_halt_wait(0x02U)) {
				// Shutting down.
				break;
			}
		} else if (errno == ECONNRESET) {
			// Shutting down.
			break;
		} else { // EOVERFLOW
			// Halt endpoint due to application being dumb.
			uep_halt(0x02U);
		}
	}

	// Free packet if we are holding onto one.
	xQueueSend(free_queue, &buf, portMAX_DELAY);

	// Done.
	xSemaphoreGive(shutdown_sem);
	vTaskDelete(0);
}

/**
 * \brief Receives unreliable message packets from OUT endpoint 3 and queues them for transmission.
 */
static void unreliable_task(void *UNUSED(param)) {
	// Run.
	packet_t *buf = 0;
	for (;;) {
		if (!buf) {
			xQueueReceive(free_queue, &buf, portMAX_DELAY);
		}
		size_t length;
		if (uep_read(0x03U, buf->data, sizeof(buf->data), &length)) {
			if (length >= 1U && buf->data[0U] < 8U) {
				buf->message_id = 0U;
				buf->reliable = false;
				buf->data_offset = 1U;
				buf->length = length - 1U;
				xQueueSend(transmit_queue, &buf, portMAX_DELAY);
				buf = 0;
			} else {
				// Halt endpoint due to application being dumb.
				uep_halt(0x03U);
			}
		} else if (errno == EPIPE) {
			// Halted.
			if (!uep_halt_wait(0x03U)) {
				// Shutting down.
				break;
			}
		} else if (errno == ECONNRESET) {
			// Shutting down.
			break;
		} else { // EOVERFLOW
			// Halt endpoint due to application being dumb.
			uep_halt(0x03U);
		}
	}

	// Free packet if we are holding onto one.
	xQueueSend(free_queue, &buf, portMAX_DELAY);

	// Done.
	xSemaphoreGive(shutdown_sem);
	vTaskDelete(0);
}

/**
 * \brief Sends queued message delivery reports to IN endpoint 1.
 */
static void mdr_task(void *UNUSED(param)) {
	bool shutting_down = false;

	// Run.
	while (!shutting_down) {
		mdr_t mdrs[4U];

		// Receive the first MDR.
		xQueueReceive(mdr_queue, &mdrs[0U], portMAX_DELAY);
		unsigned int count = 1U;

		// Receive up to three more MDRs to fill the array.
		// Do not wait for them to be ready, though.
		while (count < 4U && xQueueReceive(mdr_queue, &mdrs[count], 0U));

		// Check if any of the MDRs indicates shutdown.
		for (unsigned int i = 0U; i < count; ++i) {
			if (mdrs[i].message_id == 0xFFU && mdrs[i].status == 0xFFU) {
				count = i;
				shutting_down = true;
				break;
			}
		}

		// Run the MDRs.
		if (count) {
			if (!uep_write(0x81U, mdrs, sizeof(mdr_t) * count, false)) {
				if (errno == EPIPE) {
					// Endpoint halted.
					// Drop MDRs on the floor.
					if (!uep_halt_wait(0x81U)) {
						shutting_down = true;
					}
					while (xQueueReceive(mdr_queue, &mdrs[0U], 0U));
				} else { // ECONNRESET
					shutting_down = true;
				}
			}
		}
	}

	// Done.
	xSemaphoreGive(shutdown_sem);
	vTaskDelete(0);
}

/**
 * \brief Sends packets received from robots to IN endpoint 2.
 */
static void usbrx_task(void *UNUSED(param)) {
	// Run.
	bool shutting_down = false;
	while (!shutting_down) {
		packet_t *packet;
		xQueueReceive(receive_queue, &packet, portMAX_DELAY);
		if (packet) {
			assert(packet->data_offset == 1U);
			bool ok;
			// Keep trying until not stopped by endpoint halt.
			while (!shutting_down && !(ok = uep_write(0x82U, packet->data, packet->length, true)) && errno == EPIPE) {
				if (!uep_halt_wait(0x82U)) {
					shutting_down = true;
				}
			}
			if (!ok && errno == ECONNRESET) {
				shutting_down = true;
			}
			xQueueSend(free_queue, &packet, portMAX_DELAY);
		} else {
			// Marker NULL packet pointer.
			shutting_down = true;
		}
	}

	// Done.
	xSemaphoreGive(shutdown_sem);
	vTaskDelete(0);
}

/**
 * \brief Sends hardware run switch state changes to IN endpoint 3.
 */
static void estop_task(void *UNUSED(param)) {
	// Set the notification semaphore.
	estop_set_sem(estop_sem);

	// Run.
	bool shutting_down = false;
	while (!shutting_down) {
		// Send the state first.
		uint8_t state = estop_read();
		bool ok;
		while (!(ok = uep_write(0x83U, &state, sizeof(state), false)) && errno == EPIPE) {
			if (!uep_halt_wait(0x83U)) {
				shutting_down = true;
			}
		}
		if (!ok && errno == ECONNRESET) {
			shutting_down = true;
		}

		// Wait for the next change of state.
		if (!shutting_down) {
			xSemaphoreTake(estop_sem, portMAX_DELAY);
		}
	}

	// Remove the notification semaphore.
	estop_set_sem(0);

	// Done.
	xSemaphoreGive(shutdown_sem);
	vTaskDelete(0);
}

/**
 * \brief Copies an outbound packet into the radio’s transmit FIFO.
 *
 * \param[in] packet the packet to prepare
 */
static void prep_send_message_packet(const packet_t *packet) {
	// Write the packet into the transmit buffer.
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 0U, 9U); // Header length
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 1U, 9U + packet->length); // Frame length
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 2U, 0b01100001U); // Frame control LSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 3U, 0b10001000U); // Frame control MSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 4U, ++mrf_tx_seqnum); // Sequence number
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 5U, radio_config.pan_id); // Destination PAN ID LSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 6U, radio_config.pan_id >> 8U); // Destination PAN ID MSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 7U, packet->data[0U] & 0xFU); // Destination address LSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 8U, 0x00U); // Destination address MSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 9U, 0x00U); // Source address LSB
	mrf_write_long(MRF_REG_LONG_TXNFIFO + 10U, 0x01U); // Source address MSB
	for (size_t i = 0U; i < packet->length; ++i) {
		mrf_write_long(MRF_REG_LONG_TXNFIFO + 11U + i, packet->data[packet->data_offset + i]);
	}
}

/**
 * \brief Sends packets from the transmit queue to the radio.
 */
static void rdtx_task(void *UNUSED(param)) {
	for (;;) {
		packet_t *packet;
		xQueueReceive(transmit_queue, &packet, portMAX_DELAY);
		if (packet) {
			xSemaphoreTake(transmit_mutex, portMAX_DELAY);
			prep_send_message_packet(packet);
			bool reliable = packet->reliable;
			unsigned int tries = 20U;
			mdr_t mdr = { .message_id = packet->message_id, .status = 0U };
			xQueueSend(free_queue, &packet, portMAX_DELAY);
			packet = 0;
			do {
				// Initiate transmission with acknowledgement.
				mrf_write_short(MRF_REG_SHORT_TXNCON, 0b00000101U);
				xSemaphoreTake(transmit_complete_sem, portMAX_DELAY);
				uint8_t txstat = mrf_read_short(MRF_REG_SHORT_TXSTAT);
				if (txstat & 0x01) {
					// Transmission failed.
					if (txstat & (1 << 5)) {
						// CCA failed.
						mdr.status = MDR_STATUS_NO_CLEAR_CHANNEL;
					} else {
						// Assume: No ACK.
#warning possibly bad assumption; add a code for "unknown reason"
						mdr.status = MDR_STATUS_NOT_ACKNOWLEDGED;
					}
				} else {
					// Transmission successful.
					mdr.status = MDR_STATUS_OK;
				}
			} while (mdr.status != MDR_STATUS_OK && --tries);
			if (reliable) {
				xQueueSend(mdr_queue, &mdr, portMAX_DELAY);
			}
			xSemaphoreGive(transmit_mutex);
		} else {
			// Shutting down.
			break;
		}
	}

	// Done.
	xSemaphoreGive(shutdown_sem);
	vTaskDelete(0);
}

/**
 * \brief Handles interrupts from the radio.
 *
 * This task handles both transmission-complete interrupts (by notifying the transmitting task) and reception-complete interrupts (by reading out the packet and placing it in the receive queue).
 */
static void rdrx_task(void *UNUSED(param)) {
	uint16_t seqnum[8U];
	for (size_t i = 0U; i < sizeof(seqnum) / sizeof(*seqnum); ++i) {
		seqnum[i] = 0xFFFFU;
	}

	for (;;) {
		// Wait for an interrupt to occur.
		xSemaphoreTake(mrf_int_sem, portMAX_DELAY);
		if (__atomic_load_n(&rdrx_shutting_down, __ATOMIC_RELAXED)) {
			break;
		}

		while (mrf_get_interrupt()) {
			// Check outstanding interrupts.
			uint8_t intstat = mrf_read_short(MRF_REG_SHORT_INTSTAT);
			if (intstat & (1U << 3U)) {
				// RXIF = 1; packet received.
				mrf_write_short(MRF_REG_SHORT_BBREG1, 0x04U); // RXDECINV = 1; invert receiver symbol sign to prevent further packet reception
				// Read out the frame length byte and frame control word.
				uint8_t rxfifo_frame_length = mrf_read_long(MRF_REG_LONG_RXFIFO);
				uint16_t frame_control = mrf_read_long(MRF_REG_LONG_RXFIFO + 1U) | (mrf_read_long(MRF_REG_LONG_RXFIFO + 2U) << 8U);
				// Sanity-check the frame control word.
				if (((frame_control >> 0U) & 7U) == 1U /* Data packet */ && ((frame_control >> 3U) & 1U) == 0U /* No security */ && ((frame_control >> 6U) & 1U) == 1U /* Intra-PAN */ && ((frame_control >> 10U) & 3U) == 2U /* 16-bit destination address */ && ((frame_control >> 14U) & 3U) == 2U /* 16-bit source address */) {
					static const uint8_t HEADER_LENGTH = 2U /* Frame control */ + 1U /* Seq# */ + 2U /* Dest PAN */ + 2U /* Dest */ + 2U /* Src */;
					static const uint8_t FOOTER_LENGTH = 2U /* Frame check sequence */;

					// Sanity-check the total frame length.
					packet_t *buffer;
					if (HEADER_LENGTH + FOOTER_LENGTH <= rxfifo_frame_length && rxfifo_frame_length <= HEADER_LENGTH + sizeof(buffer->data) - 1U /* Robot index */ - 1U /* LQI */ - 1U /* RSSI */ + FOOTER_LENGTH) {
						// Read out and check the source address and sequence number.
						uint16_t source_address = mrf_read_long(MRF_REG_LONG_RXFIFO + 8U) | (mrf_read_long(MRF_REG_LONG_RXFIFO + 9U) << 8U);
						uint8_t sequence_number = mrf_read_long(MRF_REG_LONG_RXFIFO + 3U);
						if (source_address < 8U && sequence_number != seqnum[source_address]) {
							// Blink the receive light.
							gpio_toggle(GPIOB, 14U);

							// Update sequence number.
							seqnum[source_address] = sequence_number;

							// Allocate a packet buffer.
							xQueueReceive(free_queue, &buffer, portMAX_DELAY);

							// Fill in the packet buffer.
							buffer->message_id = 0U;
							buffer->reliable = false;
							buffer->data_offset = 1U;
							buffer->length = 1U /* Robot index */ + (rxfifo_frame_length - HEADER_LENGTH - FOOTER_LENGTH) /* 802.15.4 packet payload */ + 1U /* LQI */ + 1U /* RSSI */;
							buffer->data[0U] = source_address;
							for (uint8_t i = 0U; i < rxfifo_frame_length - HEADER_LENGTH - FOOTER_LENGTH; ++i) {
								buffer->data[1U + i] = mrf_read_long(MRF_REG_LONG_RXFIFO + 1U /* Frame length */ + HEADER_LENGTH + i);
							}
							buffer->data[1U + rxfifo_frame_length - HEADER_LENGTH - FOOTER_LENGTH] = mrf_read_long(MRF_REG_LONG_RXFIFO + 1U /* Frame length */ + rxfifo_frame_length); // LQI
							buffer->data[1U + rxfifo_frame_length - HEADER_LENGTH - FOOTER_LENGTH + 1U] = mrf_read_long(MRF_REG_LONG_RXFIFO + 1U /* Frame length */ + rxfifo_frame_length + 1U); // RSSI

							// Push the packet on the receive queue.
							xQueueSend(receive_queue, &buffer, portMAX_DELAY);
						}
					}
				}
				mrf_write_short(MRF_REG_SHORT_BBREG1, 0x00U); // RXDECINV = 0; stop inverting receiver and allow further reception
			}
			if (intstat & (1 << 0)) {
				// TXIF = 1; transmission complete (successful or failed).
				xSemaphoreGive(transmit_complete_sem);
			}
		}
	}

	// Done.
	xSemaphoreGive(shutdown_sem);
	vTaskDelete(0);
}

bool normal_can_enter(void) {
	return radio_config.pan_id != 0xFFFFU;
}

void normal_on_enter(void) {
	// Initialize data structures.
	rdrx_shutting_down = false;
	free_queue = xQueueCreate(NUM_PACKETS, sizeof(packet_t *));
	transmit_queue = xQueueCreate(NUM_PACKETS + 1U, sizeof(packet_t *));
	receive_queue = xQueueCreate(NUM_PACKETS + 1U, sizeof(packet_t *));
	mdr_queue = xQueueCreate(NUM_PACKETS + 1U, sizeof(mdr_t));
	estop_sem = xSemaphoreCreateBinary();
	mrf_int_sem = xSemaphoreCreateBinary();
	transmit_mutex = xSemaphoreCreateMutex();
	transmit_complete_sem = xSemaphoreCreateBinary();
	drive_event_group = xEventGroupCreate();
	shutdown_sem = xSemaphoreCreateCounting(8U /* Eight tasks */, 0U);
	assert(free_queue && transmit_queue && receive_queue && mdr_queue && estop_sem && mrf_int_sem && transmit_mutex && transmit_complete_sem && drive_event_group && shutdown_sem);

	// Allocate packet buffers.
	for (unsigned int i = 0U; i < NUM_PACKETS; ++i) {
		packet_t *packet = malloc(sizeof(packet_t));
		xQueueSend(free_queue, &packet, portMAX_DELAY);
	}

	// Initialize the radio.
	mrf_init();
	vTaskDelay(1U);
	mrf_release_reset();
	vTaskDelay(1U);
	mrf_common_init();
	while (mrf_get_interrupt());
	mrf_write_short(MRF_REG_SHORT_SADRH, 0x01U);
	mrf_write_short(MRF_REG_SHORT_SADRL, 0x00U);
	mrf_analogue_txrx();
	mrf_write_short(MRF_REG_SHORT_INTCON, 0b11110110);

	// Turn on LED 1.
	gpio_set(GPIOB, 12U);

	// Enable external interrupt on MRF INT rising edge.
	mrf_enable_interrupt(&mrf_int_isr, EXCEPTION_MKPRIO(6U, 0U));

	// Start tasks.
	BaseType_t ret = xTaskCreate(&drive_task, "norm_drive", 1024U, 0, 7U, 0);
	assert(ret == pdPASS);
	ret = xTaskCreate(&reliable_task, "norm_reliable", 1024U, 0, 6U, 0);
	assert(ret == pdPASS);
	ret = xTaskCreate(&unreliable_task, "norm_unreliable", 1024U, 0, 6U, 0);
	assert(ret == pdPASS);
	ret = xTaskCreate(&mdr_task, "norm_mdr", 1024U, 0, 5U, 0);
	assert(ret == pdPASS);
	ret = xTaskCreate(&usbrx_task, "norm_usbrx", 1024U, 0, 6U, 0);
	assert(ret == pdPASS);
	ret = xTaskCreate(&estop_task, "norm_estop", 1024U, 0, 5U, 0);
	assert(ret == pdPASS);
	ret = xTaskCreate(&rdtx_task, "norm_rdtx", 1024U, 0, 7U, 0);
	assert(ret == pdPASS);
	ret = xTaskCreate(&rdrx_task, "norm_rdrx", 1024U, 0, 7U, 0);
	assert(ret == pdPASS);
}

void normal_on_exit(void) {
	// Push markers into the queues to notify the tasks.
	static packet_t * const null_packet = 0;
	xQueueSend(transmit_queue, &null_packet, portMAX_DELAY);
	xQueueSend(receive_queue, &null_packet, portMAX_DELAY);
	static const mdr_t null_mdr = { 0xFFU, 0xFFU };
	xQueueSend(mdr_queue, &null_mdr, portMAX_DELAY);
	xSemaphoreGive(estop_sem);
	__atomic_store_n(&rdrx_shutting_down, true, __ATOMIC_RELAXED);
	xSemaphoreGive(mrf_int_sem);

	// Wait for the tasks to terminate.
	for (unsigned int i = 0U; i != 8U; ++i) {
		xSemaphoreTake(shutdown_sem, portMAX_DELAY);
	}

	// Disable the external interrupt on MRF INT.
	mrf_disable_interrupt();

	// Turn off all LEDs.
	gpio_set_reset_mask(GPIOB, 0U, 7U << 12U);

	// Reset the radio.
	mrf_deinit();

	// Free packet buffers.
	{
		packet_t *packet;
		while (xQueueReceive(free_queue, &packet, 0U)) {
			free(packet);
		}
		while (xQueueReceive(transmit_queue, &packet, 0U)) {
			free(packet);
		}
		while (xQueueReceive(receive_queue, &packet, 0U)) {
			free(packet);
		}
	}

	// Destroy data structures.
	vSemaphoreDelete(shutdown_sem);
	vEventGroupDelete(drive_event_group);
	vSemaphoreDelete(transmit_complete_sem);
	vSemaphoreDelete(mrf_int_sem);
	vSemaphoreDelete(transmit_mutex);
	vSemaphoreDelete(estop_sem);
	vQueueDelete(mdr_queue);
	vQueueDelete(receive_queue);
	vQueueDelete(transmit_queue);
	vQueueDelete(free_queue);

	// Give the timer task some time to free task stacks.
	vTaskDelay(1U);
}

