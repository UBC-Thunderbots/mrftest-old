/**
 * \defgroup UEP Nonzero endpoint handling
 *
 * These functions allow the application to communicate over a nonzero endpoint.
 * None of the function in this section may be called from the USB stack internal task unless otherwise indicated.
 *
 * @{
 */
#include <usb.h>
#include "internal.h"
#include <FreeRTOS.h>
#include <errno.h>
#include <limits.h>
#include <minmax.h>
#include <stdlib.h>
#include <task.h>
#include <registers/otg_fs.h>

/**
 * \cond INTERNAL
 * \brief The configuration and status data for the OUT endpoints.
 */
uep_ep_t uep_eps[UEP_MAX_ENDPOINT * 2U];

/**
 * \brief The maximum number of packets in an OUT transfer.
 *
 * This value is determined by the width of the \c PKTCNT field in the \c TSIZ register.
 */
#define OUT_PXFR_MAX_PACKETS 0x3FFU

/**
 * \brief The maximum number of bytes in an OUT transfer.
 *
 * This value is determined by the width of the \c XFRSIZ field in the \c TSIZ register.
 */
#define OUT_PXFR_MAX_BYTES 0x7FFFFU

/**
 * \brief The maximum number of packets in an IN transfer.
 *
 * This value is determined by the width of the \c PKTCNT field in the \c TSIZ register.
 */
#define IN_PXFR_MAX_PACKETS 0x3FFU

/**
 * \brief The maximum number of bytes in an IN transfer.
 *
 * This value should be determined by the width of the \c XFRSIZ field in the \c TSIZ register.
 * However, what appears to be a hardware bug means that transfers occasionally execute at the wrong size if bit 11 or above of the \c XFRSIZ field is nonzero!
 */
#define IN_PXFR_MAX_BYTES 0x7FFFFU

/**
 * \endcond
 */



/**
 * \cond INTERNAL
 */
/**
 * \brief Sends a notification to the asynchronous API event group for an endpoint, if one is present.
 *
 * \param[in] ctx the endpoint to notify
 */
void uep_notify_async(uep_ep_t *ctx) {
	taskENTER_CRITICAL();
	EventGroupHandle_t group = ctx->async_group;
	EventBits_t bits = ctx->async_bits;
	taskEXIT_CRITICAL();
	if (group) {
		xEventGroupSetBits(group, bits);
	}
}

/**
 * \brief Sends a notification to the asynchronous API event group for an endpoint, if one is present, from an interrupt service routine.
 *
 * \param[in] ctx the endpoint to notify
 *
 * \param[out] yield set to \c pdTRUE if the current task should yield due to the notification
 */
void uep_notify_async_from_isr(uep_ep_t *ctx, BaseType_t *yield) {
	if (ctx->async_group) {
		BaseType_t ok = xEventGroupSetBitsFromISR(ctx->async_group, ctx->async_bits, yield);
		assert(ok);
	}
}

static bool uep_read_impl(unsigned int ep, void *buffer, size_t max_length, size_t *length) {
	uep_ep_t *ctx = &uep_eps[UEP_IDX(ep)];

	// Check for endpoint deactivation or halt.
	EventBits_t events = xEventGroupGetBits(ctx->event_group) & (UEP_EVENT_DEACTIVATED | UEP_EVENT_HALTED);
	if (events) {
		if (length) {
			*length = 0U;
		}
		errno = (events & UEP_EVENT_DEACTIVATED) ? ECONNRESET : EPIPE;
		return false;
	}

	// Sanity check.
	assert(OTG_FS.DOEP[UEP_NUM(ep) - 1U].DOEPCTL.USBAEP);

	// Set up the control structure.
	ctx->data.out = buffer;
	ctx->bytes_left = max_length;
	ctx->bytes_transferred = 0U;
	ctx->flags.zlp = false;
	ctx->flags.overflow = false;

	// Grab maximum packet size.
	size_t max_packet = OTG_FS.DOEP[UEP_NUM(ep) - 1U].DOEPCTL.MPSIZ;

	// A transfer is complete when:
	// (1) the expected number of bytes has been received,
	// (2) a short packet is received
	//
	// A short packet is always either a packet which is not a multiple of max packet size or a ZLP.
	while (ctx->bytes_left && !(ctx->bytes_transferred % max_packet) && !ctx->flags.zlp) {
		// Start a physical transfer.
		// The byte count field must always be set to a multiple of the maximum packet size for an OUT endpoint.
		// Overflow detection is handled in the ISR, not in the hardware.
		size_t pxfr_packets = (ctx->bytes_left + max_packet - 1U) / max_packet;
		pxfr_packets = MIN(pxfr_packets, OUT_PXFR_MAX_PACKETS);
		pxfr_packets = MIN(pxfr_packets, OUT_PXFR_MAX_BYTES / max_packet);
		OTG_FS_DOEPTSIZx_t tsiz = { .PKTCNT = pxfr_packets, .XFRSIZ = pxfr_packets * max_packet };
		OTG_FS.DOEP[UEP_NUM(ep) - 1U].DOEPTSIZ = tsiz;
		OTG_FS_DOEPCTLx_t ctl = OTG_FS.DOEP[UEP_NUM(ep) - 1U].DOEPCTL;
		assert(!ctl.EPENA);
		ctl.EPENA = 1;
		ctl.CNAK = 1;
		ctl.STALL = 0;
		__atomic_signal_fence(__ATOMIC_ACQ_REL); // Prevent writes to context block from being sunk below this point, nor the write to CTL from being hoisted, which could break the ISR.
		OTG_FS.DOEP[UEP_NUM(ep) - 1U].DOEPCTL = ctl;

		// Wait for something to happen.
		do {
			events = xEventGroupWaitBits(ctx->event_group, UEP_EVENT_XFRC | UEP_EVENT_DEACTIVATED | UEP_EVENT_HALTED, pdTRUE, pdFALSE, portMAX_DELAY);
		} while (!(events & (UEP_EVENT_XFRC | UEP_EVENT_DEACTIVATED | UEP_EVENT_HALTED)));

		if (events & (UEP_EVENT_DEACTIVATED | UEP_EVENT_HALTED)) {
			// The endpoint is being deactivated or halted.
			// Begin by taking global OUT NAK.
			udev_gonak_take();
			// Check the status of the endpoint.
			ctl = OTG_FS.DOEP[UEP_NUM(ep) - 1U].DOEPCTL;
			if (ctl.EPENA) {
				// Endpoint is still enabled.
				// Force-disable it.
				ctl.EPDIS = 1;
				ctl.SNAK = 1;
				OTG_FS.DOEP[UEP_NUM(ep) - 1U].DOEPCTL = ctl;
				while (!(xEventGroupWaitBits(ctx->event_group, UEP_EVENT_DISD, pdTRUE, pdFALSE, portMAX_DELAY) & UEP_EVENT_DISD));
			}
			// Release global OUT NAK.
			udev_gonak_release();
			// Endpoint is now fully disabled.
			if (length) {
				*length = ctx->bytes_transferred;
			}
			xEventGroupSetBits(ctx->event_group, events & (UEP_EVENT_DEACTIVATED | UEP_EVENT_HALTED));
			errno = (events & UEP_EVENT_DEACTIVATED) ? ECONNRESET : EPIPE;
			return false;
		}
	}

	if (length) {
		*length = ctx->bytes_transferred;
	}
	if (ctx->flags.overflow) {
		errno = EOVERFLOW;
		return false;
	} else {
		return true;
	}
}
/**
 * \endcond
 */

/**
 * \brief Reads a block of data from an OUT endpoint.
 *
 * This function blocks until the requested amount of data is received, a short transaction is received, or an error is detected.
 *
 * \param[in] ep the endpoint address to read from, from 0x01 to \ref UEP_MAX_ENDPOINT
 *
 * \param[out] buffer the buffer into which to store the received data
 *
 * \param[in] max_length the maximum number of bytes to receive
 *
 * \param[out] length the actual number of bytes received (which is valid and may be nonzero even if an error occurs)
 *
 * \retval true if the read completed successfully
 * \retval false if an error occurred before or during the read (despite which some data may have been received)
 *
 * \exception EPIPE the endpoint was halted, either when the function was first called or while the transfer was occurring
 * \exception ECONNRESET the endpoint was disabled, either when the function was first called or while the transfer was occurring
 * \exception EOVERFLOW \p max_length is not a multiple of the endpoint maximum packet size and the last transaction did not fit in the buffer
 *
 * \pre The endpoint must be enabled in the current configuration and in the current alternate setting of any relevant interfaces.
 */
bool uep_read(unsigned int ep, void *buffer, size_t max_length, size_t *length) {
	assert(!UEP_DIR(ep));
	assert(1 <= UEP_NUM(ep) && UEP_NUM(ep) <= UEP_MAX_ENDPOINT);
	assert(buffer);
	assert(max_length);

	uep_ep_t *ctx = &uep_eps[UEP_IDX(ep)];
	xSemaphoreTake(ctx->transfer_mutex, portMAX_DELAY);
	bool ret = uep_read_impl(ep, buffer, max_length, length);
	xSemaphoreGive(ctx->transfer_mutex);
	return ret;
}

/**
 * \cond INTERNAL
 */

/**
 * \brief Starts a physical transfer on an IN endpoint.
 *
 * This function chooses the specifications of a physical transfer, writes those specifications into the context block, and enables the hardware.
 *
 * \param[in] ep the endpoint address to start
 *
 * \pre The endpoint is disabled.
 * \pre The \ref uep_ep_t::zlp "zlp" flag and the \ref uep_ep_t::bytes_left "bytes_left" and \ref uep_ep_t::data "data" fields are set properly.
 * \post The endpoint is enabled, with the ISR ready to push data if needed.
 * \post The \ref uep_ep_t::pxfr_bytes_left "pxfr_bytes_left" field is set properly.
 */
static void uep_write_pxfr_start(unsigned int ep) {
	uep_ep_t *ctx = &uep_eps[UEP_IDX(ep)];

	// Grab the control register, sanity check, and extract max packet size.
	OTG_FS_DIEPCTLx_t ctl = OTG_FS.DIEP[UEP_NUM(ep) - 1U].DIEPCTL;
	assert(!ctl.EPENA);
	size_t max_packet = ctl.MPSIZ;

	// Compute the size of the physical transfer.
	// This is computed based on three constraints:
	// (1) the amount of data left in the logical transfer,
	// (2) the maximum number of bytes per physical transfer (rounded down to a max-packet boundary),
	// (3) the maximum number of packets per physical transfer
	size_t pxfr_bytes = MIN(ctx->bytes_left, MIN(IN_PXFR_MAX_PACKETS, IN_PXFR_MAX_BYTES / max_packet) * max_packet);

	// Start a physical transfer.
	//
	// To start a physical transfer, we must do four things:
	// (1) Set DIEPTSIZ.
	// (2) Set DIEPCTL.
	// (3) Set pxfr_bytes_left.
	// (4) Set INEPTXFEM.
	//
	// The USB engine requires that DIEPTSIZ be set before enabling an endpoint; therefore, (2) must be done after (1).
	//
	// The ISR considers the endpoint to be a candidate for pushing data whenever pxfr_bytes_left is nonzero (this is the canonical record of readiness for pushing).
	// The USB engine requires that an endpoint be enabled before any data is pushed into its FIFO.
	// Therefore, (3) must be done after (2).
	//
	// The ISR may clear a bit in INEPTXFEM if its corresponding pxfr_bytes_left is zero when an interrupt is taken.
	// To ensure this is not done spuriously, (4) must be done after (3).
	OTG_FS_DIEPTSIZx_t tsiz;
	if (pxfr_bytes) {
		tsiz.XFRSIZ = pxfr_bytes;
		tsiz.PKTCNT = (pxfr_bytes + max_packet - 1U) / max_packet;
	} else {
		tsiz.XFRSIZ = 0U;
		tsiz.PKTCNT = 1U;
		ctx->flags.zlp = false;
	}
	OTG_FS.DIEP[UEP_NUM(ep) - 1U].DIEPTSIZ = tsiz;
	ctl.EPENA = 1;
	ctl.CNAK = 1;
	ctl.STALL = 0;
	OTG_FS.DIEP[UEP_NUM(ep) - 1U].DIEPCTL = ctl;
	__atomic_signal_fence(__ATOMIC_RELEASE); // Prevent prior writes from being sunk below pxfr_bytes_left write.
	__atomic_store_n(&ctx->pxfr_bytes_left, pxfr_bytes, __ATOMIC_RELAXED);
	__atomic_signal_fence(__ATOMIC_ACQUIRE); // Prevent subsequent writes from being hoisted above pxfr_bytes_left write.
	if (pxfr_bytes) {
		taskENTER_CRITICAL();
		OTG_FS.DIEPEMPMSK.INEPTXFEM |= 1U << UEP_NUM(ep);
		taskEXIT_CRITICAL();
	}
}

/**
 * \brief Aborts a running physical transfer on an endpoint.
 *
 * This function is used in cases of abnormal termination of a transfer (namely, endpoint disablement or halting).
 *
 * \param[in] ep the endpoint address to abort
 *
 * \post The endpoint is disabled.
 * \post The \ref uep_ep_t::pxfr_bytes_left "pxfr_bytes_left" field is zero.
 */
static void uep_write_pxfr_abort(unsigned int ep) {
	uep_ep_t *ctx = &uep_eps[UEP_IDX(ep)];

	// Begin by zeroing out the physical transfer size so the ISR will stop pushing data.
	__atomic_store_n(&ctx->pxfr_bytes_left, 0U, __ATOMIC_RELAXED);
	__atomic_signal_fence(__ATOMIC_ACQUIRE); // Prevent subsequent writes from being hoisted above pxfr_bytes_left write.

	// Obtain local NAK status.
	xEventGroupClearBits(ctx->event_group, UEP_IN_EVENT_NAKEFF);
	OTG_FS_DIEPCTLx_t ctl = OTG_FS.DIEP[UEP_NUM(ep) - 1U].DIEPCTL;
	if (!ctl.NAKSTS) {
		ctl.SNAK = 1;
		OTG_FS.DIEP[UEP_NUM(ep) - 1U].DIEPCTL = ctl;
		while (!(xEventGroupWaitBits(ctx->event_group, UEP_IN_EVENT_NAKEFF, pdTRUE, pdFALSE, portMAX_DELAY) & UEP_IN_EVENT_NAKEFF));
	}

	// Check the status of the endpoint.
	ctl = OTG_FS.DIEP[UEP_NUM(ep) - 1U].DIEPCTL;
	if (ctl.EPENA) {
		// Endpoint is still enabled.
		// Force-disable it.
		ctl.EPDIS = 1;
		OTG_FS.DIEP[UEP_NUM(ep) - 1U].DIEPCTL = ctl;
		while (!(xEventGroupWaitBits(ctx->event_group, UEP_EVENT_DISD, pdTRUE, pdFALSE, portMAX_DELAY) & UEP_EVENT_DISD));
	}
}

static bool uep_write_impl(unsigned int ep, const void *data, size_t length, bool zlp) {
	uep_ep_t *ctx = &uep_eps[UEP_IDX(ep)];

	// Check for endpoint deactivation or halt.
	EventBits_t events = xEventGroupGetBits(ctx->event_group) & (UEP_EVENT_DEACTIVATED | UEP_EVENT_HALTED);
	if (events) {
		errno = (events & UEP_EVENT_DEACTIVATED) ? ECONNRESET : EPIPE;
		return false;
	}

	// Sanity check.
	assert(OTG_FS.DIEP[UEP_NUM(ep) - 1U].DIEPCTL.USBAEP);

	// Grab the maximum packet size.
	size_t max_packet = OTG_FS.DIEP[UEP_NUM(ep) - 1U].DIEPCTL.MPSIZ;

	// Decide whether we need a ZLP.
	ctx->flags.zlp = zlp && !(length % max_packet);

	// Set up the control structure.
	ctx->data.in = data;
	ctx->bytes_left = length;
	ctx->bytes_transferred = 0U;

	while (ctx->bytes_left || ctx->flags.zlp) {
		// Start a physical transfer.
		uep_write_pxfr_start(ep);

		// Wait for something to happen.
		do {
			events = xEventGroupWaitBits(ctx->event_group, UEP_EVENT_XFRC | UEP_EVENT_DEACTIVATED | UEP_EVENT_HALTED, pdTRUE, pdFALSE, portMAX_DELAY);
		} while (!(events & (UEP_EVENT_XFRC | UEP_EVENT_DEACTIVATED | UEP_EVENT_HALTED)));

		if (events & (UEP_EVENT_DEACTIVATED | UEP_EVENT_HALTED)) {
			// The endpoint is being deactivated or halted.
			uep_write_pxfr_abort(ep);
			xEventGroupSetBits(ctx->event_group, events & (UEP_EVENT_DEACTIVATED | UEP_EVENT_HALTED));
			errno = (events & UEP_EVENT_DEACTIVATED) ? ECONNRESET : EPIPE;
			return false;
		}
	}

	return true;
}
/**
 * \endcond
 */

/**
 * \brief Writes a block of data to an IN endpoint.
 *
 * This function blocks until the data has been delivered or an error is detected.
 *
 * \param[in] ep the endpoint to write to, from 0x81 to <code>\ref UEP_MAX_ENDPOINT | 0x80</code>
 *
 * \param[in] data the data to send
 *
 * \param[in] length the number of bytes to send
 *
 * \param[in] zlp \c true to add a zero-length packet if \p length is a multiple of the endpoint maximum packet size, or \c false to omit the zero-length packet
 *
 * \retval true if the write completed successfully
 * \retval false if an error occurred before or during the write (despite which some data may have been sent)
 *
 * \exception EPIPE the endpoint was halted, either when the function was first called or while the transfer was occurring
 * \exception ECONNRESET the endpoint was disabled, either when the function was first called or while the transfer was occurring
 *
 * \pre The endpoint must be enabled in the current configuration and in the current alternate setting of any relevant interfaces.
 */
bool uep_write(unsigned int ep, const void *data, size_t length, bool zlp) {
	assert(UEP_DIR(ep));
	assert(1 <= UEP_NUM(ep) && UEP_NUM(ep) <= UEP_MAX_ENDPOINT);
	assert(!!data == !!length);
	assert(length || zlp);

	uep_ep_t *ctx = &uep_eps[UEP_IDX(ep)];

	xSemaphoreTake(ctx->transfer_mutex, portMAX_DELAY);
	bool ret = uep_write_impl(ep, data, length, zlp);
	xSemaphoreGive(ctx->transfer_mutex);
	return ret;
}

/**
 * \brief Waits for halt status to clear on a halted endpoint.
 *
 * This function returns only when the endpoint becomes disabled or when halt status is successfully cleared on the endpoint.
 * If the endpoint is not halted, this function returns immediately.
 *
 * \param[in] ep the endpoint on which to wait, from 0x01 to \ref UEP_MAX_ENDPOINT or 0x81 to <code>\ref UEP_MAX_ENDPOINT | 0x80</code>
 *
 * \retval true if endpoint halt status has been cleared and the endpoint is now working normally
 * \retval false if an error occurred
 *
 * \exception ECONNRESET the endpoint was disabled, either when the function was first called or while the transfer was occurring
 *
 * \pre The endpoint must be enabled in the current configuration and in the current alternate setting of any relevant interfaces.
 */
bool uep_halt_wait(unsigned int ep) {
	assert(UEP_NUM(ep) && UEP_NUM(ep) <= UEP_MAX_ENDPOINT);
	uep_ep_t *ctx = &uep_eps[UEP_IDX(ep)];
	EventBits_t events;
	do {
		events = xEventGroupWaitBits(ctx->event_group, UEP_EVENT_DEACTIVATED | UEP_EVENT_NOT_HALTED, pdFALSE, pdFALSE, portMAX_DELAY);
	} while (!(events & (UEP_EVENT_DEACTIVATED | UEP_EVENT_NOT_HALTED)));
	if (events & UEP_EVENT_DEACTIVATED) {
		errno = ECONNRESET;
		return false;
	} else {
		return true;
	}
}

/**
 * \cond INTERNAL
 */
static void uep_async_read_start_pxfr(unsigned int ep) {
	uep_ep_t *ctx = &uep_eps[UEP_IDX(ep)];
	size_t max_packet = OTG_FS.DOEP[UEP_NUM(ep) - 1U].DOEPCTL.MPSIZ;

	// Start a physical transfer.
	// The byte count field must always be set to a multiple of the maximum packet size for an OUT endpoint.
	// Overflow detection is handled in the ISR, not in the hardware.
	size_t pxfr_packets = (ctx->bytes_left + max_packet - 1U) / max_packet;
	pxfr_packets = MIN(pxfr_packets, OUT_PXFR_MAX_PACKETS);
	pxfr_packets = MIN(pxfr_packets, OUT_PXFR_MAX_BYTES / max_packet);
	OTG_FS_DOEPTSIZx_t tsiz = { .PKTCNT = pxfr_packets, .XFRSIZ = pxfr_packets * max_packet };
	OTG_FS.DOEP[UEP_NUM(ep) - 1U].DOEPTSIZ = tsiz;
	OTG_FS_DOEPCTLx_t ctl = OTG_FS.DOEP[UEP_NUM(ep) - 1U].DOEPCTL;
	assert(!ctl.EPENA);
	ctl.EPENA = 1;
	ctl.CNAK = 1;
	ctl.STALL = 0;
	OTG_FS.DOEP[UEP_NUM(ep) - 1U].DOEPCTL = ctl;
}
/**
 * \endcond
 */

/**
 * \brief Starts an asynchronous read on an endpoint.
 *
 * This function returns immediately, allowing the read to continue in the background.
 * The memory referred to by the \p buffer pointer \em must remain valid until the asynchronous read operation completes (successfully or otherwise)!
 *
 * \param[in] ep the endpoint to read from, from 0x01 to \ref UEP_MAX_ENDPOINT
 *
 * \param[out] buffer the buffer into which to store the received data
 *
 * \param[in] max_length the maximum number of bytes to receive
 *
 * \param[in] group the group to notify when the endpoint needs servicing
 *
 * \param[in] bits the bits to set when the endpoint needs servicing
 *
 * \retval true if the read started successfully
 * \retval false if an error occurred before the read started (in which case no data has been received)
 *
 * \exception EPIPE the endpoint was halted at the time of call
 * \exception ECONNRESET the endpoint was disabled at the time of call
 *
 * \pre The endpoint must be enabled in the current configuration and in the current alternate setting of any relevant interfaces.
 */
bool uep_async_read_start(unsigned int ep, void *buffer, size_t max_length, EventGroupHandle_t group, EventBits_t bits) {
	assert(!UEP_DIR(ep));
	assert(1 <= UEP_NUM(ep) && UEP_NUM(ep) <= UEP_MAX_ENDPOINT);
	assert(buffer);
	assert(max_length);
	assert(group);
	assert(bits);

	uep_ep_t *ctx = &uep_eps[UEP_IDX(ep)];

	// Lock the transfer mutex so we can do a transfer.
	xSemaphoreTake(ctx->transfer_mutex, portMAX_DELAY);

	// Sanity check.
	assert(OTG_FS.DOEP[UEP_NUM(ep) - 1U].DOEPCTL.USBAEP);

	// Set up the control structures.
	ctx->data.out = buffer;
	ctx->bytes_left = max_length;
	ctx->bytes_transferred = 0U;
	ctx->flags.zlp = false;
	ctx->flags.overflow = false;
	taskENTER_CRITICAL();
	ctx->async_group = group;
	ctx->async_bits = bits;
	taskEXIT_CRITICAL();

	// Check for endpoint deactivation or halt.
	// This must be done after the store to ctx->async_group.
	// Otherwise, another task’s request for deactivate or halt might not make its way into the async group, yet we might consider the operation to have started.
	// By putting the check here, we are guaranteed that any deactivate or halt occurring after this check will also set bits in group.
	EventBits_t events = xEventGroupGetBits(ctx->event_group) & (UEP_EVENT_DEACTIVATED | UEP_EVENT_HALTED);
	if (events) {
		__atomic_store_n(&ctx->async_group, 0, __ATOMIC_RELAXED);
		xSemaphoreGive(ctx->transfer_mutex);
		errno = (events & UEP_EVENT_DEACTIVATED) ? ECONNRESET : EPIPE;
		return false;
	}

	// Start a physical transfer.
	uep_async_read_start_pxfr(ep);

	return true;
}

/**
 * \brief Finishes an asynchronous read on an endpoint.
 *
 * This function returns immediately, whether or not the read was finished, reporting the operation’s current status.
 *
 * \param[in] ep the endpoint being read from, from 0x01 to \ref UEP_MAX_ENDPOINT
 *
 * \param[out] length the number of bytes actually received
 *
 * \retval true if the read completed successfully
 * \retval false if an error occurred while the read was in progress (despite which some data may have been received), if the read has not yet finished, or if no asynchronous read was started
 *
 * \exception EPIPE the endpoint was halted while the read was in progress
 * \exception ECONNRESET the endpoint was disabled while the read was in progress
 * \exception EOVERFLOW \p max_length was not a multiple of the endpoint maximum packet size and the last transaction did not fit in the buffer
 * \exception EINPROGRESS the operation has not yet finished
 *
 * \pre A prior asynchronous read must have been started and not yet reported completed on this endpoint.
 */
bool uep_async_read_finish(unsigned int ep, size_t *length) {
	assert(!UEP_DIR(ep));
	assert(1 <= UEP_NUM(ep) && UEP_NUM(ep) <= UEP_MAX_ENDPOINT);

	uep_ep_t *ctx = &uep_eps[UEP_IDX(ep)];

	// Check what, if anything, happened.
	EventBits_t events = xEventGroupClearBits(ctx->event_group, UEP_EVENT_XFRC | UEP_EVENT_DEACTIVATED | UEP_EVENT_HALTED);
	if (events & (UEP_EVENT_DEACTIVATED | UEP_EVENT_HALTED)) {
		// The endpoint is being deactivated or halted.
		// Begin by taking global OUT NAK.
		udev_gonak_take();
		// Check the status of the endpoint.
		OTG_FS_DOEPCTLx_t ctl = OTG_FS.DOEP[UEP_NUM(ep) - 1U].DOEPCTL;
		if (ctl.EPENA) {
			// Endpoint is still enabled.
			// Force-disable it.
			ctl.EPDIS = 1;
			ctl.SNAK = 1;
			OTG_FS.DOEP[UEP_NUM(ep) - 1U].DOEPCTL = ctl;
			while (!(xEventGroupWaitBits(ctx->event_group, UEP_EVENT_DISD, pdTRUE, pdFALSE, portMAX_DELAY) & UEP_EVENT_DISD));
		}
		// Release global OUT NAK.
		udev_gonak_release();
		// Endpoint is now fully disabled.
		if (length) {
			*length = ctx->bytes_transferred;
		}
		xEventGroupSetBits(ctx->event_group, events & (UEP_EVENT_DEACTIVATED | UEP_EVENT_HALTED));
		__atomic_store_n(&ctx->async_group, 0, __ATOMIC_RELAXED);
		xSemaphoreGive(ctx->transfer_mutex);
		errno = (events & UEP_EVENT_DEACTIVATED) ? ECONNRESET : EPIPE;
		return false;
	} else if (events & UEP_EVENT_XFRC) {
		// A physical transfer finished.
		// A transfer is complete when:
		// (1) the expected number of bytes has been received,
		// (2) a short packet is received
		//
		// A short packet is always either a packet which is not a multiple of max packet size or a ZLP.
		size_t max_packet = OTG_FS.DOEP[UEP_NUM(ep) - 1U].DOEPCTL.MPSIZ;
		if (ctx->bytes_left && !(ctx->bytes_transferred % max_packet) && ctx->flags.zlp) {
			// The logical transfer is not finished yet.
			uep_async_read_start_pxfr(ep);
			errno = EINPROGRESS;
			return false;
		} else {
			// The logical transfer is finished.
			if (length) {
				*length = ctx->bytes_transferred;
			}
			bool overflow = !!ctx->flags.overflow;
			__atomic_store_n(&ctx->async_group, 0, __ATOMIC_RELAXED);
			xSemaphoreGive(ctx->transfer_mutex);
			if (overflow) {
				errno = EOVERFLOW;
				return false;
			} else {
				return true;
			}
		}
	} else {
		// Nothing happened.
		errno = EINPROGRESS;
		return false;
	}
}

/**
 * \brief Starts an asynchronous write on an endpoint.
 *
 * This function returns immediately, allowing the write to continue in the background.
 * The memory referred to by the \p data pointer \em must remain valid until the asynchronous write operation completes (successfully or otherwise)!
 *
 * \param[in] ep the endpoint to write to, from 0x81 to <code>\ref UEP_MAX_ENDPOINT | 0x80</code>
 *
 * \param[in] data the data to send
 *
 * \param[in] length the number of bytes to send
 *
 * \param[in] zlp \c true to add a zero-length packet if \p length is a multiple of the endpoint maximum packet size, or \c false to omit the zero-length packet
 *
 * \param[in] group the group to notify when the endpoint needs servicing
 *
 * \param[in] bits the bits to set when the endpoint needs servicing
 *
 * \retval true if the write started successfully
 * \retval false if an error occurred before the write started (in which case no data has been sent)
 *
 * \exception EPIPE the endpoint was halted at the time of call
 * \exception ECONNRESET the endpoint was disabled at the time of call
 *
 * \pre The endpoint must be enabled in the current configuration and in the current alternate setting of any relevant interfaces.
 */
bool uep_async_write_start(unsigned int ep, const void *data, size_t length, bool zlp, EventGroupHandle_t group, EventBits_t bits) {
	assert(UEP_DIR(ep));
	assert(1 <= UEP_NUM(ep) && UEP_NUM(ep) <= UEP_MAX_ENDPOINT);
	assert(!!data == !!length);
	assert(length || zlp);
	assert(group);
	assert(bits);

	uep_ep_t *ctx = &uep_eps[UEP_IDX(ep)];

	// Lock the transfer mutex so we can do a transfer.
	xSemaphoreTake(ctx->transfer_mutex, portMAX_DELAY);

	// Sanity check.
	assert(OTG_FS.DIEP[UEP_NUM(ep) - 1U].DIEPCTL.USBAEP);

	// Grab the maximum packet size.
	size_t max_packet = OTG_FS.DIEP[UEP_NUM(ep) - 1U].DIEPCTL.MPSIZ;

	// Set up the control structure.
	ctx->data.in = data;
	ctx->bytes_left = length;
	ctx->bytes_transferred = 0U;
	ctx->flags.zlp = zlp && !(length % max_packet);
	taskENTER_CRITICAL();
	ctx->async_group = group;
	ctx->async_bits = bits;
	taskEXIT_CRITICAL();

	// Check for endpoint deactivation or halt.
	// This must be done after the store to ctx->async_group.
	// Otherwise, another task’s request for deactivate or halt might not make its way into the async group, yet we might consider the operation to have started.
	// By putting the check here, we are guaranteed that any deactivate or halt occurring after this check will also set bits in group.
	EventBits_t events = xEventGroupGetBits(ctx->event_group) & (UEP_EVENT_DEACTIVATED | UEP_EVENT_HALTED);
	if (events) {
		__atomic_store_n(&ctx->async_group, 0, __ATOMIC_RELAXED);
		xSemaphoreGive(ctx->transfer_mutex);
		errno = (events & UEP_EVENT_DEACTIVATED) ? ECONNRESET : EPIPE;
		return false;
	}

	// Start a physical transfer.
	uep_write_pxfr_start(ep);

	return true;
}

/**
 * \brief Finishes an asynchronous write on an endpoint.
 *
 * This function returns immediately, whether or not the write was finished, reporting the operation’s current status.
 *
 * \param[in] ep the endpoint being read from, from 0x81 to <code>\ref UEP_MAX_ENDPOINT | 0x80</code>
 *
 * \retval true if the write completed successfully
 * \retval false if an error occurred while the write was in progress (despite which some data may have been sent), if the write has not yet finished, or if no asynchronous write was started
 *
 * \exception EPIPE the endpoint was halted while the read was in progress
 * \exception ECONNRESET the endpoint was disabled while the read was in progress
 * \exception EINPROGRESS the operation has not yet finished
 *
 * \pre A prior asynchronous write must have been started and not yet reported completed on this endpoint.
 */
bool uep_async_write_finish(unsigned int ep) {
	assert(UEP_DIR(ep));
	assert(1 <= UEP_NUM(ep) && UEP_NUM(ep) <= UEP_MAX_ENDPOINT);

	uep_ep_t *ctx = &uep_eps[UEP_IDX(ep)];

	// Check what, if anything, happened.
	EventBits_t events = xEventGroupClearBits(ctx->event_group, UEP_EVENT_XFRC | UEP_EVENT_DEACTIVATED | UEP_EVENT_HALTED);
	if (events & (UEP_EVENT_DEACTIVATED | UEP_EVENT_HALTED)) {
		// The endpoint is being deactivated or halted.
		uep_write_pxfr_abort(ep);
		xEventGroupSetBits(ctx->event_group, events & (UEP_EVENT_DEACTIVATED | UEP_EVENT_HALTED));
		__atomic_store_n(&ctx->async_group, 0, __ATOMIC_RELAXED);
		xSemaphoreGive(ctx->transfer_mutex);
		errno = (events & UEP_EVENT_DEACTIVATED) ? ECONNRESET : EPIPE;
		return false;
	} else if (events & UEP_EVENT_XFRC) {
		// A physical transfer finished.
		if (ctx->bytes_left || ctx->flags.zlp) {
			// The logical transfer is not finished yet.
			uep_write_pxfr_start(ep);
			errno = EINPROGRESS;
			return false;
		} else {
			// The logical transfer is finished.
			__atomic_store_n(&ctx->async_group, 0, __ATOMIC_RELAXED);
			xSemaphoreGive(ctx->transfer_mutex);
			return true;
		}
	} else {
		// Nothing happened.
		errno = EINPROGRESS;
		return false;
	}
}

/**
 * \brief Starts waiting for halt status to clear on a halted endpoint.
 *
 * This function returns immediately, allowing the wait to continue in the background.
 * If the endpoint is not halted, this function succeeds and starts a wait which immediately finishes.
 *
 * \param[in] ep the endpoint on which to wait, from 0x01 to \ref UEP_MAX_ENDPOINT or 0x81 to <code>\ref UEP_MAX_ENDPOINT | 0x80</code>
 *
 * \param[in] group the group to notify when the endpoint needs servicing
 *
 * \param[in] bits the bits to set when the endpoint needs servicing
 *
 * \retval true if the wait started successfully
 * \retval false if an error occurred before the wait started
 *
 * \exception ECONNRESET the endpoint was disabled at the time of call
 *
 * \pre The endpoint must be enabled in the current configuration and in the current alternate setting of any relevant interfaces.
 */
bool uep_async_halt_wait_start(unsigned int ep, EventGroupHandle_t group, EventBits_t bits) {
	assert(1 <= UEP_NUM(ep) && UEP_NUM(ep) <= UEP_MAX_ENDPOINT);
	assert(group);
	assert(bits);

	uep_ep_t *ctx = &uep_eps[UEP_IDX(ep)];

	// Set up the control structures.
	taskENTER_CRITICAL();
	ctx->async_group = group;
	ctx->async_bits = bits;
	taskEXIT_CRITICAL();

	// Check for endpoint deactivation.
	// This must be done after the store to ctx->async_group.
	// Otherwise, another task’s request for deactivate or clear halt might not make its way into the async group, yet we might consider the operation to have started.
	// By putting the check here, we are guaranteed that any deactivate or clear halt occurring after this check will also set bits in group.
	if (xEventGroupGetBits(ctx->event_group) & UEP_EVENT_DEACTIVATED) {
		__atomic_store_n(&ctx->async_group, 0, __ATOMIC_RELAXED);
		errno = ECONNRESET;
		return false;
	}

	return true;
}

/**
 * \brief Finishes waiting for halt status to clear on a halted endpoint.
 *
 * This function returns immediately, whether or not the wait was finished, reporting the operation’s current status.
 *
 * \param[in] ep the endpoint being waited on, from 0x01 to \ref UEP_MAX_ENDPOINT or 0x81 to <code>\ref UEP_MAX_ENDPOINT | 0x80</code>
 *
 * \retval true if the endpoint halt status has been cleared and the endpoint is now working normally
 * \retval false if an error occurred while the wait was in progress, if the endpoint is still halted, or if no asynchronous wait was started
 *
 * \exception ECONNRESET the endpoint was disabled while the wait was in progress
 * \exception EINPROGRESS the operation has not yet finished
 *
 * \pre A prior asynchronous halt wait must have been started and not yet reported completed on this endpoint.
 */
bool uep_async_halt_wait_finish(unsigned int ep) {
	assert(1 <= UEP_NUM(ep) && UEP_NUM(ep) <= UEP_MAX_ENDPOINT);

	uep_ep_t *ctx = &uep_eps[UEP_IDX(ep)];

	// Check what, if anything, happened.
	EventBits_t events = xEventGroupGetBits(ctx->event_group);
	if (events & UEP_EVENT_DEACTIVATED) {
		// The endpoint is being deactivated.
		__atomic_store_n(&ctx->async_group, 0, __ATOMIC_RELAXED);
		errno = ECONNRESET;
		return false;
	} else if (events & UEP_EVENT_NOT_HALTED) {
		// The endpoint is no longer halted.
		__atomic_store_n(&ctx->async_group, 0, __ATOMIC_RELAXED);
		return true;
	} else {
		// Nothing happened.
		errno = EINPROGRESS;
		return false;
	}
}

/**
 * \cond INTERNAL
 */
static void uep_halt_impl(unsigned int ep) {
	uep_ep_t *ctx = &uep_eps[UEP_IDX(ep)];

	// If endpoint is not active, do nothing.
	if (xEventGroupGetBits(ctx->event_group) & UEP_EVENT_DEACTIVATED) {
		return;
	}

	// Mark endpoint as halted in control block, which causes any in-progress transfer to terminate.
	xEventGroupClearBits(ctx->event_group, UEP_EVENT_NOT_HALTED);
	xEventGroupSetBits(ctx->event_group, UEP_EVENT_HALTED);

	// Notify the asynchronous event group if one is present.
	uep_notify_async(ctx);

	// Wait for in-progress transfer to terminate and then take the transfer mutex to prevent another from starting.
	xSemaphoreTake(ctx->transfer_mutex, portMAX_DELAY);

	// Set the hardware.
	if (UEP_DIR(ep)) {
		OTG_FS.DIEP[UEP_NUM(ep) - 1U].DIEPCTL.STALL = 1;
	} else {
		OTG_FS.DOEP[UEP_NUM(ep) - 1U].DOEPCTL.STALL = 1;
	}

	// Release transfer mutex.
	xSemaphoreGive(ctx->transfer_mutex);
}
/**
 * \endcond
 */

/**
 * \brief Sets the halt feature on an endpoint.
 *
 * This function blocks until the feature is fully set.
 *
 * There is no corresponding \c uep_unhalt function because halt status can only be cleared by request from the host system.
 *
 * \param[in] ep the endpoint to halt, from 0x01 to \ref UEP_MAX_ENDPOINT or 0x81 to <code>\ref UEP_MAX_ENDPOINT | 0x80</code>
 */
void uep_halt(unsigned int ep) {
	assert(1 <= UEP_NUM(ep) && UEP_NUM(ep) <= UEP_MAX_ENDPOINT);

	uep_ep_t *ctx = &uep_eps[UEP_IDX(ep)];

	xSemaphoreTake(ctx->manage_mutex, portMAX_DELAY);
	uep_halt_impl(ep);
	xSemaphoreGive(ctx->manage_mutex);
}

/**
 * \cond INTERNAL
 */
static void uep_activate_impl(const usb_endpoint_descriptor_t *edesc, unsigned int interface) {
	uep_ep_t *ctx = &uep_eps[UEP_IDX(edesc->bEndpointAddress)];

	// Set up state block.
	ctx->interface = interface;
	xEventGroupClearBits(ctx->event_group, UEP_EVENT_DISD | UEP_IN_EVENT_NAKEFF | UEP_EVENT_XFRC | UEP_EVENT_DEACTIVATED);

	// Enable the endpoint hardware.
	if (UEP_DIR(edesc->bEndpointAddress)) {
		// The amount of space available for this FIFO is MAX{16, transmit_fifo_words[N]} words.
		// We cannot use more than that amount, because other FIFOs lie beyond that space.
		// However, we can elect to use a smaller FIFO if it would help.
		//
		// Normally, a larger FIFO helps reduce interrupts; multiple packets can be pushed at once, and an interrupt is taken only once half of them have been transmitted.
		// However, the hardware requires that, no matter how large the FIFO is, only seven packets may be present at any time.
		// Fortunately, the half-empty interrupt is inhibited if all seven packets are present, even if the FIFO is less than half full, avoiding an interrupt storm.
		// As the FIFO grows beyond seven max-packets, however, interrupt frequency increases.
		// This is because fewer packets have to be transmitted in order for the FIFO to become half empty.
		// In the limit, once the FIFO reaches 12 packets in size, every packet transmitted brings the FIFO from seven to six packets, making it half empty and causing an interrupt.
		// Absolute minimization of interrupts occurs when the FIFO is seven packets long, so we try to do that.
		// However, we must still obey the other hardware rule, namely that the FIFO must be at least 16 words long.
		// That means we cannot quite achieve seven packets for a max packet size of eight bytes or smaller; however, we do the best we can.
		//
		// All of the above is completely irrelevant if the interrupt minimization flag is set, because then the interrupt fires when the FIFO is completely empty.
		size_t max_packet_words = (edesc->wMaxPacketSize + 3U) / 4U;
		size_t alloc_words = MAX(16U, uep0_current_configuration->transmit_fifo_words[UEP_NUM(edesc->bEndpointAddress) - 1U]);
		size_t fifo_words;
		if (udev_info->flags.minimize_interrupts) {
			fifo_words = alloc_words;
		} else {
			fifo_words = MAX(16U, MIN(alloc_words, max_packet_words * 7U));
		}
		OTG_FS.DIEPTXF[UEP_NUM(edesc->bEndpointAddress) - 1U].INEPTXFD = fifo_words;

		// The FIFO must be large enough to contain at least one packet.
		assert(fifo_words >= max_packet_words);

		// Flush transmit FIFO.
		udev_flush_tx_fifo(edesc->bEndpointAddress);

		// Activate endpoint.
		assert(!OTG_FS.DIEP[UEP_NUM(edesc->bEndpointAddress) - 1U].DIEPCTL.USBAEP);
		OTG_FS_DIEPCTLx_t ctl = {
			.SD0PID_SEVNFRM = 1,
			.TXFNUM = UEP_NUM(edesc->bEndpointAddress),
			.EPTYP = edesc->bmAttributes.type,
			.USBAEP = 1,
			.MPSIZ = edesc->wMaxPacketSize,
		};
		OTG_FS.DIEP[UEP_NUM(edesc->bEndpointAddress) - 1U].DIEPCTL = ctl;
	} else {
		// Activate endpoint.
		assert(!OTG_FS.DOEP[UEP_NUM(edesc->bEndpointAddress) - 1U].DOEPCTL.USBAEP);
		OTG_FS_DOEPCTLx_t ctl = {
			.SD0PID_SEVENFRM = 1,
			.EPTYP = edesc->bmAttributes.type,
			.USBAEP = 1,
			.MPSIZ = edesc->wMaxPacketSize,
		};
		OTG_FS.DOEP[UEP_NUM(edesc->bEndpointAddress) - 1U].DOEPCTL = ctl;
	}
}

/**
 * \brief Activates an endpoint.
 *
 * \param[in] edesc the endpoint descriptor describing the endpoint to activate
 *
 * \param[in] interface the interface number to which the endpoint belongs, or UINT_MAX if none
 */
void uep_activate(const usb_endpoint_descriptor_t *edesc, unsigned int interface) {
	assert(UEP_NUM(edesc->bEndpointAddress) && UEP_NUM(edesc->bEndpointAddress) <= UEP_MAX_ENDPOINT);

	uep_ep_t *ctx = &uep_eps[UEP_IDX(edesc->bEndpointAddress)];

	xSemaphoreTake(ctx->manage_mutex, portMAX_DELAY);
	uep_activate_impl(edesc, interface);
	xSemaphoreGive(ctx->manage_mutex);
}

static void uep_deactivate_impl(const usb_endpoint_descriptor_t *edesc) {
	uep_ep_t *ctx = &uep_eps[UEP_IDX(edesc->bEndpointAddress)];

	// Mark the endpoint as deactivated, signalling any running operation to terminate.
	xEventGroupSetBits(ctx->event_group, UEP_EVENT_DEACTIVATED);

	// Notify the asynchronous event group if one is present.
	uep_notify_async(ctx);

	// Wait for in-progress transfer to terminate and then take the transfer mutex to prevent another from starting.
	xSemaphoreTake(ctx->transfer_mutex, portMAX_DELAY);

	// Disable the endpoint hardware.
	if (UEP_DIR(edesc->bEndpointAddress)) {
		assert(!OTG_FS.DIEP[UEP_NUM(edesc->bEndpointAddress) - 1U].DIEPCTL.EPENA);
		OTG_FS_DIEPCTLx_t ctl = { 0 };
		OTG_FS.DIEP[UEP_NUM(edesc->bEndpointAddress) - 1U].DIEPCTL = ctl;
	} else {
		assert(!OTG_FS.DOEP[UEP_NUM(edesc->bEndpointAddress) - 1U].DOEPCTL.EPENA);
		OTG_FS_DOEPCTLx_t ctl = { 0 };
		OTG_FS.DOEP[UEP_NUM(edesc->bEndpointAddress) - 1U].DOEPCTL = ctl;
	}

	// Clear the state block and release the transfer mutex.
	ctx->interface = UINT_MAX;
	xEventGroupClearBits(ctx->event_group, UEP_EVENT_HALTED);
	xEventGroupSetBits(ctx->event_group, UEP_EVENT_NOT_HALTED);
	xSemaphoreGive(ctx->transfer_mutex);
}

/**
 * \brief Deactivates an endpoint.
 *
 * \param[in] edesc the endpoint descriptor describing the endpoint to deactivate
 *
 * \pre This function must be invoked on the stack internal task.
 */
void uep_deactivate(const usb_endpoint_descriptor_t *edesc) {
	assert(UEP_NUM(edesc->bEndpointAddress) && UEP_NUM(edesc->bEndpointAddress) <= UEP_MAX_ENDPOINT);

	uep_ep_t *ctx = &uep_eps[UEP_IDX(edesc->bEndpointAddress)];

	xSemaphoreTake(ctx->manage_mutex, portMAX_DELAY);
	uep_deactivate_impl(edesc);
	xSemaphoreGive(ctx->manage_mutex);
}
/**
 * \endcond
 */

/**
 * @}
 */

