#include "promiscuous.h"
#include "constants.h"
#include "led.h"
#include "mrf.h"
#include "radio_config.h"
#include <FreeRTOS.h>
#include <errno.h>
#include <minmax.h>
#include <queue.h>
#include <rcc.h>
#include <semphr.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <unused.h>
#include <usb.h>
#include <registers/exti.h>

typedef struct {
	uint8_t length;
	uint8_t data[1U + 1U + 127U]; // Flags + channel + data (RX FIFO is 128 bytes long, of which 1 is used for frame length)
} packet_t;

#define NUM_PACKETS 256U

static QueueHandle_t free_queue, receive_queue;
static SemaphoreHandle_t event_sem, init_shutdown_sem;
static TaskHandle_t radio_task_handle, usb_task_handle;
static uint16_t promisc_flags;
static bool shutting_down;

static void mrf_int_isr(void) {
	// Clear the interrupt and give the semaphore.
	EXTI.PR = 1U << 12U; // PR12 = 1; clear pending EXTI12 interrupt
	BaseType_t yield = pdFALSE;
	xSemaphoreGiveFromISR(event_sem, &yield);
	if (yield) {
		portYIELD_FROM_ISR();
	}
}

static void radio_task(void *UNUSED(param)) {
	for (;;) {
		// Wait to be instructed to start doing work.
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		// Initialize the radio.
		mrf_init();
		vTaskDelay(1U);
		mrf_release_reset();
		vTaskDelay(1U);
		mrf_common_init();
		while (mrf_get_interrupt());

		// Enable external interrupt on MRF INT rising edge.
		mrf_enable_interrupt(&mrf_int_isr);

		// Notify on_enter that initialization is finished.
		xSemaphoreGive(init_shutdown_sem);

		// Run the main operation.
		bool packet_dropped = false;
		while (!__atomic_load_n(&shutting_down, __ATOMIC_RELAXED)) {
			while (mrf_get_interrupt()) {
				// Check outstanding interrupts.
				uint8_t intstat = mrf_read_short(MRF_REG_SHORT_INTSTAT);
				if (intstat & (1U << 3U)) {
					// RXIF = 1; packet received.
					mrf_write_short(MRF_REG_SHORT_BBREG1, 0x04U); // RXDECINV = 1; invert receiver symbol sign to prevent further packet reception.
					uint8_t rxfifo_frame_length = mrf_read_long(MRF_REG_LONG_RXFIFO); // Need to read this here even if no packet buffer because this read also re-enables the receiver.

					// Sanitize the frame length to avoid buffer overruns.
					rxfifo_frame_length = MIN(rxfifo_frame_length, 128U /* RX FIFO size */ - 1U /* Length */ - 1U /* LQI */ - 1U /* RSSI */);

#warning proper packet filtering when radio filters do not match capture flags exactly

					// Allocate a packet.
					packet_t *packet;
					if (xQueueReceive(free_queue, &packet, 0U)) {
						packet->length = 1U /* Flags */ + 1U /* Channel */ + rxfifo_frame_length /* MAC header + data + FCS */ + 1U /* LQI */ + 1U /* RSSI */;
						packet->data[0U] = packet_dropped ? 0x01U : 0x00U;
						packet_dropped = false;
						packet->data[1U] = radio_config.channel;
						for (size_t i = 0U; i < rxfifo_frame_length + 2U /* LQI + RSSI */; ++i) {
							packet->data[2U + i] = mrf_read_long(MRF_REG_LONG_RXFIFO + i + 1U);
						}
						xQueueSend(receive_queue, &packet, portMAX_DELAY);
					} else {
						packet_dropped = true;
					}
					mrf_write_short(MRF_REG_SHORT_BBREG1, 0x00U); // RXDECINV = 0; stop inverting receiver and allow further reception

					// Blink receive LED.
					led_blink(LED_RX);
				}
			}

			xSemaphoreTake(event_sem, portMAX_DELAY);
		}

		// Disable the external interrupt on MRF INT.
		mrf_disable_interrupt();

		// Reset the radio.
		mrf_deinit();

		// Done.
		xSemaphoreGive(init_shutdown_sem);
	}
}

static void usb_task(void *UNUSED(param)) {
	for (;;) {
		// Wait to be instructed to start doing work.
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		// Run.
		for (;;) {
			// Get a received packet.
			packet_t *packet;
			xQueueReceive(receive_queue, &packet, portMAX_DELAY);
			if (packet) {
				// Send the packet over USB.
				bool ok = uep_write(0x81U, packet->data, packet->length, true);
				int error_code = errno;
				xQueueSend(free_queue, &packet, portMAX_DELAY);
				if (!ok && error_code == ECONNRESET) {
					// Shutdown signal.
					break;
				}
				// In case of EPIPE (endpoint halt status), drop packets on the floor until the host changes its mind.
			} else {
				// Pushing a null pointer into the queue signals shutdown.
				break;
			}
		}

		xSemaphoreGive(init_shutdown_sem);
	}
}

void promiscuous_init(void) {
	// Create IPC objects.
	free_queue = xQueueCreate(NUM_PACKETS, sizeof(packet_t *));
	receive_queue = xQueueCreate(NUM_PACKETS + 1U /* Signalling NULL */, sizeof(packet_t *));
	event_sem = xSemaphoreCreateBinary();
	init_shutdown_sem = xSemaphoreCreateCounting(2U /* Two tasks */, 0U);
	assert(free_queue && receive_queue && event_sem && init_shutdown_sem);

	// Allocate packet buffers.
	static packet_t packets[NUM_PACKETS];
	for (unsigned int i = 0U; i < NUM_PACKETS; ++i) {
		packet_t *packet = &packets[i];
		xQueueSend(free_queue, &packet, portMAX_DELAY);
	}

	// Create tasks.
	BaseType_t ret = xTaskCreate(&radio_task, "prom_radio", 1024U, 0, 6U, &radio_task_handle);
	assert(ret == pdPASS);
	ret = xTaskCreate(&usb_task, "prom_usb", 1024U, 0, 5U, &usb_task_handle);
	assert(ret == pdPASS);
}

void promiscuous_on_enter(void) {
	// Clear flags.
	promisc_flags = 0U;
	shutting_down = false;

	// Tell the tasks to start doing work.
	xTaskNotifyGive(radio_task_handle);
	xTaskNotifyGive(usb_task_handle);

	// Wait until the task has finished initializing the radio.
	// We must do this because we need to prevent SET PROMISCUOUS FLAGS from arriving and poking things during initialization.
	// During initialization, this could be avoided by the task taking the bus mutex.
	// However, that would leave a race where SET PROMISCUOUS FLAGS arrives before the task gets around to taking said mutex.
	// This would be safe as far as mutual exclusion on the bus is concerned.
	// However, it would have a different problem: the promiscuous flags would be destroyed by the initialization routine.
	//
	// One might think this could be avoided by taking the mutex here and passing ownership of it into the task.
	// However, FreeRTOS semaphores can be given and taken by different tasks, but mutexes cannot.
	// A mutex must always be given by the same task that takes it.
	// So, we can’t do that.
	// Instead, we implement this initialization-waiter mechanism.
	xSemaphoreTake(init_shutdown_sem, portMAX_DELAY);
}

void promiscuous_on_exit(void) {
	// Shut down tasks.
	__atomic_store_n(&shutting_down, true, __ATOMIC_RELAXED);
	__atomic_signal_fence(__ATOMIC_RELEASE);
	xSemaphoreGive(event_sem);
	static packet_t * const null_packet = 0;
	xQueueSend(receive_queue, &null_packet, portMAX_DELAY);
	xSemaphoreTake(init_shutdown_sem, portMAX_DELAY);
	xSemaphoreTake(init_shutdown_sem, portMAX_DELAY);

	// Flush receive queue.
	packet_t *packet;
	while (xQueueReceive(receive_queue, &packet, 0)) {
		xQueueSend(free_queue, &packet, 0);
	}

	// Turn off receive LED.
	led_off(LED_RX);
}

bool promiscuous_control_handler(const usb_setup_packet_t *pkt) {
	if (pkt->bmRequestType.direction && pkt->bmRequestType.recipient == USB_RECIPIENT_INTERFACE && pkt->bmRequestType.type == USB_CTYPE_VENDOR && pkt->bRequest == CONTROL_REQUEST_GET_PROMISCUOUS_FLAGS && !pkt->wValue) {
		uep0_data_write(&promisc_flags, sizeof(promisc_flags));
		return true;
	} else if (pkt->bmRequestType.recipient == USB_RECIPIENT_INTERFACE && pkt->bmRequestType.type == USB_CTYPE_VENDOR && pkt->bRequest == CONTROL_REQUEST_SET_PROMISCUOUS_FLAGS && !pkt->wLength) {
		// This request must have value using only bits 0 through 7.
		if (pkt->wValue & 0xFF00U) {
			return false;
		}

		// Check if the application actually wants *ANY* packets.
		if (pkt->wValue & 0xF0U) {
			// Sanity check: if flag 0 (acknowledge) is set and one of bits 4 through 7 (accept some type of frame) is set…
			if ((pkt->wValue & 0x0001U) && (pkt->wValue & 0xF0U)) {
				// … either exactly one or all three of bits 4 through 6 must be set to 1…
				if ((pkt->wValue & 0x70U) != 0x10U && (pkt->wValue & 0x70U) != 0x20U && (pkt->wValue & 0x70U) != 0x40U && (pkt->wValue & 0x70U) != 0x70U) {
					return false;
				}
				// … either none or all of bits 1, 2, and 7 must be set to 1…
				if ((pkt->wValue & 0x86U) != 0x00U && (pkt->wValue & 0x86U) != 0x86U) {
					return false;
				}
				// … and if bit 7 is set to 1, bits 4 through 6 must also all be set to 1
				if ((pkt->wValue & 0x80U) && (pkt->wValue & 0x70U) != 0x70U) {
					return false;
				}
			}
			// This set of flags is acceptable; save.
			promisc_flags = pkt->wValue;
			// Disable all packet reception.
			mrf_write_short(MRF_REG_SHORT_BBREG1, 0x04U);
			// Install the new flags.
			mrf_write_short(MRF_REG_SHORT_RXMCR,
					((pkt->wValue & (1U << 0U)) ? 0U : (1U << 5U))
					| ((pkt->wValue & (1U << 3U)) ? (1U << 1U) : 0U)
					| ((pkt->wValue & 0x86U) ? (1U << 0U) : 0U));
			if ((pkt->wValue & 0xF0U) == 0x10U) {
				mrf_write_short(MRF_REG_SHORT_RXFLUSH, 0x64U);
			} else if ((pkt->wValue & 0xF0U) == 0x20U) {
				mrf_write_short(MRF_REG_SHORT_RXFLUSH, 0x68U);
			} else if ((pkt->wValue & 0xF0U) == 0x40U) {
				mrf_write_short(MRF_REG_SHORT_RXFLUSH, 0x62U);
			} else {
				mrf_write_short(MRF_REG_SHORT_RXFLUSH, 0x60U);
			}
			// Set analogue path appropriately based on whether ACKs are being generated and whether any packets are desired.
			if (pkt->wValue & 0x01U) {
				mrf_analogue_txrx();
			} else {
				mrf_analogue_rx();
			}
			// Re-enable packet reception.
			mrf_write_short(MRF_REG_SHORT_BBREG1, 0x00U);
			// Enable interrupt on receive.
			mrf_write_short(MRF_REG_SHORT_INTCON, 0xF7U);
			// Turn on receive LED to indicate capture is enabled.
			led_on(LED_RX);
		} else {
			// Shut down the radio.
			mrf_write_short(MRF_REG_SHORT_RXMCR, 0x20U);
			mrf_write_short(MRF_REG_SHORT_BBREG1, 0x04U);
			mrf_write_short(MRF_REG_SHORT_INTCON, 0xFFU);
			mrf_analogue_off();
			// Turn off receive LED to indicate capture is disabled.
			led_off(LED_RX);
		}

		return true;
	}

	return false;
}

