/**
 * \defgroup CDCACM Communications device class abstract control model functions
 *
 * \brief These functions allow sending data over an emulated serial port.
 *
 * CDC ACM functionality exists within a USB configuration.
 * Within that configuration, the functionality uses two USB interfaces (grouped under an interface association descriptor) and three endpoints (two IN and one OUT).
 * The application must include an instance of structure \ref cdcacm_descriptors_t in its configuration descriptor.
 * This structure should be initialized by setting its value to the result of a call to \ref CDCACM_DESCRIPTORS_INIT.
 * The device descriptor’s class, subclass, and protocol numbers must be set to indicate the use of interface association descriptors.
 *
 * At runtime, an application begins by calling \ref cdcacm_init at startup, and must do so only once.
 * Thereafter, the application must call \ref cdcacm_start when entering the relevant configuration, and \ref cdcacm_stop when exiting it, from the USB stack internal task.
 * The application may call \ref cdcacm_write at any time once \ref cdcacm_init has returned, from any task, regardless of whether CDC ACM is active or whether any other task is in any of the functions.
 *
 * @{
 */

#include <cdcacm.h>
#include <FreeRTOS.h>
#include <assert.h>
#include <errno.h>
#include <event_groups.h>
#include <minmax.h>
#include <semphr.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <task.h>
#include <unused.h>

/**
 * \cond INTERNAL
 */

/**
 * \brief The size of the data buffer.
 */
#define CDCACM_BUFFER_SIZE 4096U

/**
 * \brief The bitmasks used in \ref cdcacm_event_group.
 */
typedef enum {
	CDCACM_EVENT_NEW_DATA = 0x01U, ///< Set by \ref cdcacm_write to tell the CDC ACM task there is new data
	CDCACM_EVENT_SHUTTING_DOWN = 0x02U, ///< Set by \ref cdcacm_stop to tell the CDC ACM task to shut down
	CDCACM_EVENT_SHUTDOWN_DONE = 0x04U, ///< Set by the CDC ACM task to notify \ref cdcacm_stop that it has finished shutting down
} cdcacm_event_bits_t;

/**
 * \brief Whether the module is currently active.
 */
static bool cdcacm_enabled;

/**
 * \brief The buffer that holds data before it travels over USB.
 */
static uint8_t cdcacm_buffer[CDCACM_BUFFER_SIZE];

/**
 * \brief The index in the buffer of the next byte that the task will send over USB.
 */
static size_t cdcacm_rptr;

/**
 * \brief The index in the buffer of the next byte that a caller will write into.
 */
static size_t cdcacm_wptr;

/**
 * \brief The endpoint address of the IN data endpoint.
 */
static unsigned int cdcacm_in_data_ep;

/**
 * \brief The priority of the CDC ACM task.
 */
static unsigned int cdcacm_task_priority;

/**
 * \brief A mutex protecting the module from multiple simultaneous writers.
 */
static SemaphoreHandle_t cdcacm_writer_mutex;

/**
 * \brief An event group used by tasks to notify each other of events.
 */
static EventGroupHandle_t cdcacm_event_group;

_Static_assert(INCLUDE_vTaskSuspend == 1, "vTaskSuspend must be included, because otherwise mutex taking can time out!");

/**
 * \endcond
 */



/**
 * \cond INTERNAL
 * \brief The CDC ACM task.
 */
static void cdcacm_task(void *UNUSED(param)) {
	for (;;) {
		// Wait for something to happen.
		EventBits_t events = xEventGroupWaitBits(cdcacm_event_group, CDCACM_EVENT_NEW_DATA | CDCACM_EVENT_SHUTTING_DOWN, pdTRUE, pdFALSE, portMAX_DELAY);

		// If we are shutting down, return now.
		if (events & CDCACM_EVENT_SHUTTING_DOWN) {
			break;
		}

		// If we have data to send, send it.
		if (events & CDCACM_EVENT_NEW_DATA) {
			// Keep going until we have no more data.
			for (;;) {
				// Copy the read and write pointers to locals.
				size_t rptr = cdcacm_rptr; // Non-atomic because only modified by cdcacm_task.
				size_t wptr = __atomic_load_n(&cdcacm_wptr, __ATOMIC_RELAXED); // Atomic because modified by cdcacm_write.
				if (wptr == rptr) {
					break;
				}
				__atomic_signal_fence(__ATOMIC_ACQUIRE);

				// Send as much as we can in one contiguous block.
				size_t to_send;
				if (rptr < wptr) {
					to_send = wptr - rptr;
				} else {
					to_send = CDCACM_BUFFER_SIZE - rptr;
				}
				if (uep_write(cdcacm_in_data_ep, &cdcacm_buffer[rptr], to_send, true)) {
					// Data sent successfully; consume from buffer.
					rptr += to_send;
					if (rptr == CDCACM_BUFFER_SIZE) {
						rptr = 0U;
					}
				} else {
					// Data transmission failed; check why.
					if (errno == EPIPE) {
						// Endpoint halted; wait for halt status to end.
						if (uep_halt_wait(cdcacm_in_data_ep)) {
							// Endpoint no longer halted; discard any data written while halted.
							__atomic_signal_fence(__ATOMIC_RELEASE);
							__atomic_store_n(&cdcacm_rptr, __atomic_load_n(&cdcacm_wptr, __ATOMIC_RELAXED), __ATOMIC_RELAXED);
						} else {
							// Endpoint deactivated while waiting for halt status; fall out and observe shutdown event on next outer-loop iteration.
							break;
						}
					} else if (errno == ECONNRESET) {
						// Endpoint deactivated while writing; fall out and observe shutdown event on next outer-loop iteration.
						break;
					} else {
						// Unknown reason.
						abort();
					}
				}
				__atomic_store_n(&cdcacm_rptr, rptr, __ATOMIC_RELAXED);
			}
		}
	}

	// Clean the event group, notify the other task, and die.
	xEventGroupClearBits(cdcacm_event_group, CDCACM_EVENT_NEW_DATA | CDCACM_EVENT_SHUTTING_DOWN);
	xEventGroupSetBits(cdcacm_event_group, CDCACM_EVENT_SHUTDOWN_DONE);
	vTaskDelete(0);
}
/**
 * \endcond
 */

/**
 * \brief Initializes the CDC ACM module.
 *
 * This function must be called and must return before any other CDC ACM functions are called.
 *
 * \param[in] in_data_ep_num the endpoint number of the IN data endpoint
 *
 * \param[in] task_priority the priority of the CDC ACM task
 */
void cdcacm_init(unsigned int in_data_ep_num, unsigned int task_priority) {
	cdcacm_enabled = false;
	cdcacm_in_data_ep = 0x80U | in_data_ep_num;
	cdcacm_task_priority = task_priority;
	cdcacm_writer_mutex = xSemaphoreCreateMutex();
	cdcacm_event_group = xEventGroupCreate();
	assert(cdcacm_writer_mutex && cdcacm_event_group);
}

/**
 * \brief Activates the CDC ACM module.
 *
 * This function must be called on the USB stack internal task when the containing configuration is entered.
 * A suitable way to do this might be to use this function as the \c on_enter callback of a \c udev_alternate_setting_info_t structure.
 */
void cdcacm_start(void) {
	xSemaphoreTake(cdcacm_writer_mutex, portMAX_DELAY);
	assert(!cdcacm_enabled);
	cdcacm_enabled = true;
	cdcacm_rptr = 0U;
	cdcacm_wptr = 0U;
	BaseType_t ok = xTaskCreate(&cdcacm_task, "cdcacm", 512U, 0, cdcacm_task_priority, 0);
	assert(ok == pdPASS);
	xSemaphoreGive(cdcacm_writer_mutex);
}

/**
 * \brief Deactivates the CDC ACM module.
 *
 * This function must be called on the USB stack internal task when the containing configuration is exited.
 * A suitable way to do this might be to use this function as the \c on_exit callback of a \c udev_alternate_setting_info_t structure.
 */
void cdcacm_stop(void) {
	xSemaphoreTake(cdcacm_writer_mutex, portMAX_DELAY);
	assert(cdcacm_enabled);
	cdcacm_enabled = false;
	xEventGroupSetBits(cdcacm_event_group, CDCACM_EVENT_SHUTTING_DOWN);
	EventBits_t events = xEventGroupWaitBits(cdcacm_event_group, CDCACM_EVENT_SHUTDOWN_DONE, pdTRUE, pdFALSE, portMAX_DELAY);
	assert(events & CDCACM_EVENT_SHUTDOWN_DONE);
	xSemaphoreGive(cdcacm_writer_mutex);
}

/**
 * \brief Writes data to the serial port.
 *
 * This function may be called whether or not the CDC ACM is currently enabled.
 * Data is queued in a buffer and transmitted in the CDC ACM task, so this function returns quickly.
 * Data is lost if the buffer becomes full (due to the host not running any transactions), the endpoint is halted, or the CDC ACM is disabled.
 *
 * \param[in] data the data to write
 *
 * \param[in] length the number of bytes to write
 */
void cdcacm_write(const void *data, size_t length) {
	xSemaphoreTake(cdcacm_writer_mutex, portMAX_DELAY);

	const uint8_t *dptr = data;

	for (;;) {
		// If there is no more data to read, then we are done.
		if (!length) {
			break;
		}

		// Copy the read and write pointers to locals.
		size_t rptr = __atomic_load_n(&cdcacm_rptr, __ATOMIC_RELAXED); // Atomic because modified by cdcacm_task.
		size_t wptr = cdcacm_wptr; // Non-atomic because only modified by cdcacm_write, against which we hold a mutex.
		__atomic_signal_fence(__ATOMIC_ACQUIRE);

		// If the buffer is full, then we are done.
		if (wptr + 1U == rptr || (wptr + 1U == CDCACM_BUFFER_SIZE && !rptr)) {
			break;
		}

		// Copy some data into the buffer.
		size_t space;
		if (wptr < rptr) {
			space = rptr - wptr - 1U;
		} else {
			space = CDCACM_BUFFER_SIZE - wptr;
		}
		size_t to_copy = MIN(space, length);
		memcpy(&cdcacm_buffer[wptr], dptr, to_copy);

		// Advance.
		dptr += to_copy;
		length -= to_copy;
		wptr += to_copy;
		if (wptr == CDCACM_BUFFER_SIZE) {
			wptr = 0U;
		}
		__atomic_signal_fence(__ATOMIC_RELEASE);
		__atomic_store_n(&cdcacm_wptr, wptr, __ATOMIC_RELAXED);
	}

	// Notify the task.
	xEventGroupSetBits(cdcacm_event_group, CDCACM_EVENT_NEW_DATA);

	xSemaphoreGive(cdcacm_writer_mutex);
}

/**
 * @}
 */

